/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Meiyi & Longda on 2021/4/13.
//
#include "disk_buffer_pool.h"
#include <errno.h>
#include <string.h>

#include "common/lang/mutex.h"
#include "common/log/log.h"
#include "common/os/os.h"
#include "common/io/io.h"

using namespace common;

static const PageNum BP_HEADER_PAGE = 0;
static const int MEM_POOL_ITEM_NUM = 128;

unsigned long current_time()
{
  struct timespec tp;
  clock_gettime(CLOCK_MONOTONIC, &tp);
  return tp.tv_sec * 1000 * 1000 * 1000UL + tp.tv_nsec;
}

BPFrameManager::BPFrameManager(const char *name) : allocator_(name)
{}

RC BPFrameManager::init(int pool_num)
{
  int ret =  allocator_.init(false, pool_num);
  if (ret == 0) {
    return RC::SUCCESS;
  }
  return RC::GENERIC_ERROR;
}

RC BPFrameManager::cleanup()
{
  if (frames_.count() > 0) {
    return RC::GENERIC_ERROR;
  }

  frames_.destroy();
  return RC::SUCCESS;
}

Frame *BPFrameManager::begin_purge()
{
  Frame *frame_can_purge = nullptr;
  auto purge_finder = [&frame_can_purge](const BPFrameId &frame_id, Frame * const frame) {
    if (frame->can_purge()) {
      frame_can_purge = frame;
      return false; // false to break the progress
    }
    return true; // true continue to look up
  };
  frames_.foreach_reverse(purge_finder);
  return frame_can_purge;
}

Frame *BPFrameManager::get(int file_desc, PageNum page_num)
{
  BPFrameId frame_id(file_desc, page_num);

  std::lock_guard<std::mutex> lock_guard(lock_);
  Frame *frame = nullptr;
  (void)frames_.get(frame_id, frame);
  return frame;
}

Frame *BPFrameManager::alloc(int file_desc, PageNum page_num)
{
  BPFrameId frame_id(file_desc, page_num);

  std::lock_guard<std::mutex> lock_guard(lock_);
  Frame *frame = nullptr;
  bool found = frames_.get(frame_id, frame);
  if (found) {
    // assert (frame != nullptr);
    return nullptr; // should use get
  }

  frame = allocator_.alloc();
  if (frame != nullptr) {
    frames_.put(frame_id, frame);
  }
  return frame;
}

RC BPFrameManager::free(int file_desc, PageNum page_num, Frame *frame)
{
  BPFrameId frame_id(file_desc, page_num);

  std::lock_guard<std::mutex> lock_guard(lock_);
  Frame *frame_source = nullptr;
  bool found = frames_.get(frame_id, frame_source);
  if (!found || frame != frame_source) {
    LOG_WARN("failed to find frame or got frame not match. file_desc=%d, PageNum=%d, frame_source=%p, frame=%p",
             file_desc, page_num, frame_source, frame);
    return RC::GENERIC_ERROR;
  }

  frames_.remove(frame_id);
  allocator_.free(frame);
  return RC::SUCCESS;
}

std::list<Frame *> BPFrameManager::find_list(int file_desc)
{
  std::lock_guard<std::mutex> lock_guard(lock_);

  std::list<Frame *> frames;
  auto fetcher = [&frames, file_desc](const BPFrameId &frame_id, Frame * const frame) -> bool {
    if (file_desc == frame_id.file_desc()) {
      frames.push_back(frame);
    }
    return true;
  };
  frames_.foreach(fetcher);
  return frames;
}

////////////////////////////////////////////////////////////////////////////////
BufferPoolIterator::BufferPoolIterator()
{}
BufferPoolIterator::~BufferPoolIterator()
{}
RC BufferPoolIterator::init(DiskBufferPool &bp, PageNum start_page /* = 0 */)
{
  bitmap_.init(bp.file_header_->bitmap, bp.file_header_->page_count);
  if (start_page <= 0) {
    current_page_num_ = 0;
  } else {
    current_page_num_ = start_page;
  }
  return RC::SUCCESS;
}

bool BufferPoolIterator::has_next()
{
  return bitmap_.next_setted_bit(current_page_num_ + 1) != -1;
}

PageNum BufferPoolIterator::next()
{
  PageNum next_page = bitmap_.next_setted_bit(current_page_num_ + 1);
  if (next_page != -1) {
    current_page_num_ = next_page;
  }
  return next_page;
}

RC BufferPoolIterator::reset()
{
  current_page_num_ = 0;
  return RC::SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
DiskBufferPool::DiskBufferPool(BufferPoolManager &bp_manager, BPFrameManager &frame_manager)
  : bp_manager_(bp_manager), frame_manager_(frame_manager)
{
}

DiskBufferPool::~DiskBufferPool()
{
  close_file();
  LOG_INFO("Exit");
}

RC DiskBufferPool::open_file(const char *file_name)
{
  int fd;
  if ((fd = open(file_name, O_RDWR)) < 0) {
    LOG_ERROR("Failed to open file %s, because %s.", file_name, strerror(errno));
    return RC::IOERR_ACCESS;
  }
  LOG_INFO("Successfully open file %s.", file_name);

  file_name_ = file_name;
  file_desc_ = fd;

  RC rc = RC::SUCCESS;
  rc = allocate_frame(BP_HEADER_PAGE, &hdr_frame_);
  if (rc != RC::SUCCESS) {
    LOG_ERROR("failed to allocate frame for header. file name %s", file_name_.c_str());
    close(fd);
    file_desc_ = -1;
    return rc;
  }

  hdr_frame_->dirty_ = false;
  hdr_frame_->file_desc_ = fd;
  hdr_frame_->pin_count_ = 1;
  hdr_frame_->acc_time_ = current_time();
  if ((rc = load_page(BP_HEADER_PAGE, hdr_frame_)) != RC::SUCCESS) {
    LOG_ERROR("Failed to load first page of %s, due to %s.", file_name, strerror(errno));
    hdr_frame_->pin_count_ = 0;
    purge_frame(BP_HEADER_PAGE, hdr_frame_);
    close(fd);
    file_desc_ = -1;
    return rc;
  }

  file_header_ = (BPFileHeader *)hdr_frame_->data();

  LOG_INFO("Successfully open %s. file_desc=%d, hdr_frame=%p", file_name, file_desc_, hdr_frame_);
  return RC::SUCCESS;
}

RC DiskBufferPool::close_file()
{
  RC rc = RC::SUCCESS;
  if (file_desc_ < 0) {
    return rc;
  }

  hdr_frame_->pin_count_--;
  if ((rc = purge_all_pages()) != RC::SUCCESS) {
    hdr_frame_->pin_count_++;
    LOG_ERROR("Failed to close %s, due to failed to purge all pages.", file_name_.c_str());
    return rc;
  }

  disposed_pages.clear();

  if (close(file_desc_) < 0) {
    LOG_ERROR("Failed to close fileId:%d, fileName:%s, error:%s", file_desc_, file_name_.c_str(), strerror(errno));
    return RC::IOERR_CLOSE;
  }
  LOG_INFO("Successfully close file %d:%s.", file_desc_, file_name_.c_str());
  file_desc_ = -1;

  bp_manager_.close_file(file_name_.c_str());
  return RC::SUCCESS;
}

RC DiskBufferPool::get_this_page(PageNum page_num, Frame **frame)
{
  RC rc = RC::SUCCESS;

  Frame *used_match_frame = frame_manager_.get(file_desc_, page_num);
  if (used_match_frame != nullptr) {
    used_match_frame->pin_count_++;
    used_match_frame->acc_time_ = current_time();

    *frame = used_match_frame;
    return RC::SUCCESS;
  }

  // Allocate one page and load the data into this page
  Frame *allocated_frame = nullptr;
  if ((rc = allocate_frame(page_num, &allocated_frame)) != RC::SUCCESS) {
    LOG_ERROR("Failed to alloc frame %s:%d, due to failed to alloc page.", file_name_.c_str(), page_num);
    return rc;
  }

  allocated_frame->dirty_ = false;
  allocated_frame->file_desc_ = file_desc_;
  allocated_frame->pin_count_ = 1;
  allocated_frame->acc_time_ = current_time();
  if ((rc = load_page(page_num, allocated_frame)) != RC::SUCCESS) {
    LOG_ERROR("Failed to load page %s:%d", file_name_.c_str(), page_num);
    allocated_frame->pin_count_ = 0;
    purge_frame(page_num, allocated_frame);
    return rc;
  }

  *frame = allocated_frame;
  return RC::SUCCESS;
}

RC DiskBufferPool::allocate_page(Frame **frame)
{
  RC rc = RC::SUCCESS;

  int byte = 0, bit = 0;
  if ((file_header_->allocated_pages) < (file_header_->page_count)) {
    // There is one free page
    for (int i = 0; i < file_header_->page_count; i++) {
      byte = i / 8;
      bit = i % 8;
      if (((file_header_->bitmap[byte]) & (1 << bit)) == 0) {
        (file_header_->allocated_pages)++;
        file_header_->bitmap[byte] |= (1 << bit);
        // TODO,  do we need clean the loaded page's data?
	hdr_frame_->mark_dirty();
        return get_this_page(i, frame);
      }
    }
  }

  if (file_header_->page_count >= BPFileHeader::MAX_PAGE_NUM) {
    LOG_WARN("file buffer pool is full. page count %d, max page count %d",
	     file_header_->page_count, BPFileHeader::MAX_PAGE_NUM);
    return BUFFERPOOL_NOBUF;
  }

  PageNum page_num = file_header_->page_count;
  Frame *allocated_frame = nullptr;
  if ((rc = allocate_frame(page_num, &allocated_frame)) != RC::SUCCESS) {
    LOG_ERROR("Failed to allocate frame %s, due to no free page.", file_name_.c_str());
    return rc;
  }

  file_header_->allocated_pages++;
  file_header_->page_count++;

  byte = page_num / 8;
  bit = page_num % 8;
  file_header_->bitmap[byte] |= (1 << bit);
  hdr_frame_->mark_dirty();

  allocated_frame->dirty_ = false;
  allocated_frame->file_desc_ = file_desc_;
  allocated_frame->pin_count_ = 1;
  allocated_frame->acc_time_ = current_time();
  allocated_frame->clear_page();
  allocated_frame->page_.page_num = file_header_->page_count - 1;

  // Use flush operation to extension file
  if ((rc = flush_page(*allocated_frame)) != RC::SUCCESS) {
    LOG_WARN("Failed to alloc page %s , due to failed to extend one page.", file_name_.c_str());
    // skip return false, delay flush the extended page
    // return tmp;
  }

  *frame = allocated_frame;
  return RC::SUCCESS;
}

RC DiskBufferPool::unpin_page(Frame *frame)
{
  assert(frame->pin_count_ >= 1);

  if (--frame->pin_count_ == 0) {
    PageNum page_num = frame->page_num();
    auto pages_it = disposed_pages.find(page_num);
    if (pages_it != disposed_pages.end()) {
      LOG_INFO("Dispose file_desc:%d, page:%d", file_desc_, page_num);
      dispose_page(page_num);
      disposed_pages.erase(pages_it);
    }
  }

  return RC::SUCCESS;
}

/**
 * dispose_page will delete the data of the page of pageNum, free the page both from buffer pool and data file.
 * purge_page will purge the page of pageNum, free the page from buffer pool
 * @return
 */
RC DiskBufferPool::dispose_page(PageNum page_num)
{
  RC rc = purge_page(page_num);
  if (rc != RC::SUCCESS) {
    LOG_INFO("Dispose page %s:%d later, due to this page is being used", file_name_.c_str(), page_num);

    disposed_pages.insert(page_num);
    return rc;
  }

  hdr_frame_->dirty_ = true;
  file_header_->allocated_pages--;
  char tmp = 1 << (page_num % 8);
  file_header_->bitmap[page_num / 8] &= ~tmp;
  return RC::SUCCESS;
}

RC DiskBufferPool::purge_frame(PageNum page_num, Frame *buf)
{
  if (buf->pin_count_ > 0) {
    LOG_INFO("Begin to free page %d of %d(file id), but it's pinned, pin_count:%d.",
	     buf->page_num(), buf->file_desc_, buf->pin_count_);
    return RC::LOCKED_UNLOCK;
  }

  if (buf->dirty_) {
    RC rc = flush_page(*buf);
    if (rc != RC::SUCCESS) {
      LOG_WARN("Failed to flush page %d of %d(file desc) during purge page.", buf->page_num(), buf->file_desc_);
      return rc;
    }
  }

  LOG_DEBUG("Successfully purge frame =%p, page %d of %d(file desc)", buf, buf->page_num(), buf->file_desc_);
  frame_manager_.free(file_desc_, page_num, buf);
  return RC::SUCCESS;
}

/**
 * dispose_page will delete the data of the page of pageNum
 * force_page will flush the page of pageNum
 * @param pageNum
 * @return
 */
RC DiskBufferPool::purge_page(PageNum page_num)
{
  Frame *used_frame = frame_manager_.get(file_desc_, page_num);
  if (used_frame != nullptr) {
    return purge_frame(page_num, used_frame);
  }

  return RC::SUCCESS;
}

RC DiskBufferPool::purge_all_pages()
{
  std::list<Frame *> used = frame_manager_.find_list(file_desc_);
  for (std::list<Frame *>::iterator it = used.begin(); it != used.end(); ++it) {
    Frame *frame = *it;
    if (frame->pin_count_ > 0) {
      LOG_WARN("The page has been pinned, file_desc:%d, pagenum:%d, pin_count=%d",
	       frame->file_desc_, frame->page_.page_num, frame->pin_count_);
      continue;
    }
    if (frame->dirty_) {
      RC rc = flush_page(*frame);
      if (rc != RC::SUCCESS) {
        LOG_ERROR("Failed to flush all pages' of %s.", file_name_.c_str());
        return rc;
      }
    }
    frame_manager_.free(file_desc_, frame->page_.page_num, frame);
  }
  return RC::SUCCESS;
}

RC DiskBufferPool::check_all_pages_unpinned()
{
  std::list<Frame *> frames = frame_manager_.find_list(file_desc_);
  for (auto & frame : frames) {
    if (frame->page_num() == BP_HEADER_PAGE && frame->pin_count_ > 1) {
      LOG_WARN("This page has been pinned. file desc=%d, page num:%d, pin count=%d",
	       file_desc_, frame->page_num(), frame->pin_count_);
    } else if (frame->page_num() != BP_HEADER_PAGE && frame->pin_count_ > 0) {
      LOG_WARN("This page has been pinned. file desc=%d, page num:%d, pin count=%d",
	       file_desc_, frame->page_num(), frame->pin_count_);
    }
  }
  LOG_INFO("all pages have been checked of file desc %d", file_desc_);
  return RC::SUCCESS;
}

RC DiskBufferPool::flush_page(Frame &frame)
{
  // The better way is use mmap the block into memory,
  // so it is easier to flush data to file.

  Page &page = frame.page_;
  s64_t offset = ((s64_t)page.page_num) * sizeof(Page);
  if (lseek(file_desc_, offset, SEEK_SET) == offset - 1) {
    LOG_ERROR("Failed to flush page %lld of %d due to failed to seek %s.", offset, file_desc_, strerror(errno));
    return RC::IOERR_SEEK;
  }

  if (writen(file_desc_, &page, sizeof(Page)) != 0) {
    LOG_ERROR("Failed to flush page %lld of %d due to %s.", offset, file_desc_, strerror(errno));
    return RC::IOERR_WRITE;
  }
  frame.dirty_ = false;
  LOG_DEBUG("Flush block. file desc=%d, page num=%d", file_desc_, page.page_num);

  return RC::SUCCESS;
}

RC DiskBufferPool::flush_all_pages()
{
  std::list<Frame *> used = frame_manager_.find_list(file_desc_);
  for (Frame *frame : used) {
    RC rc = flush_page(*frame);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to flush all pages");
      return rc;
    }
  }
  return RC::SUCCESS;
}

RC DiskBufferPool::allocate_frame(PageNum page_num, Frame **buffer)
{
  while (true) {
    Frame *frame = frame_manager_.alloc(file_desc_, page_num);
    if (frame != nullptr) {
      *buffer = frame;
      return RC::SUCCESS;
    }

    frame = frame_manager_.begin_purge();
    if (frame == nullptr) {
      LOG_ERROR("All pages have been used and pinned.");
      return RC::NOMEM;
    }

    if (frame->dirty_) {
      RC rc = bp_manager_.flush_page(*frame);
      if (rc != RC::SUCCESS) {
        LOG_ERROR("Failed to aclloc block due to failed to flush old block.");
        return rc;
      }
    }

    frame_manager_.free(frame->file_desc(), frame->page_num(), frame);
  }
  return RC::INTERNAL;
}

RC DiskBufferPool::check_page_num(PageNum page_num)
{
  if (page_num >= file_header_->page_count) {
    LOG_ERROR("Invalid pageNum:%d, file's name:%s", page_num, file_name_.c_str());
    return RC::BUFFERPOOL_INVALID_PAGE_NUM;
  }
  if ((file_header_->bitmap[page_num / 8] & (1 << (page_num % 8))) == 0) {
    LOG_ERROR("Invalid pageNum:%d, file's name:%s", page_num, file_name_.c_str());
    return RC::BUFFERPOOL_INVALID_PAGE_NUM;
  }
  return RC::SUCCESS;
}

RC DiskBufferPool::load_page(PageNum page_num, Frame *frame)
{
  s64_t offset = ((s64_t)page_num) * sizeof(Page);
  if (lseek(file_desc_, offset, SEEK_SET) == -1) {
    LOG_ERROR("Failed to load page %s:%d, due to failed to lseek:%s.",
	      file_name_.c_str(), page_num, strerror(errno));

    return RC::IOERR_SEEK;
  }

  int ret = readn(file_desc_, &(frame->page_), sizeof(Page));
  if (ret != 0) {
    LOG_ERROR("Failed to load page %s:%d, due to failed to read data:%s, ret=%d, page count=%d",
	      file_name_.c_str(), page_num, strerror(errno), ret, file_header_->allocated_pages);
    return RC::IOERR_READ;
  }
  return RC::SUCCESS;
}

RC DiskBufferPool::get_page_count(int *page_count)
{
  *page_count = file_header_->allocated_pages;
  return RC::SUCCESS;
}
int DiskBufferPool::file_desc() const
{
  return file_desc_;
}
////////////////////////////////////////////////////////////////////////////////
BufferPoolManager::BufferPoolManager()
{
  frame_manager_.init(MEM_POOL_ITEM_NUM);
}

BufferPoolManager::~BufferPoolManager()
{
  std::unordered_map<std::string, DiskBufferPool *> tmp_bps;
  tmp_bps.swap(buffer_pools_);
  
  for (auto &iter : tmp_bps) {
    delete iter.second;
  }
}

RC BufferPoolManager::create_file(const char *file_name)
{
  int fd = open(file_name, O_RDWR | O_CREAT | O_EXCL, S_IREAD | S_IWRITE);
  if (fd < 0) {
    LOG_ERROR("Failed to create %s, due to %s.", file_name, strerror(errno));
    return RC::SCHEMA_DB_EXIST;
  }

  close(fd);

  /**
   * Here don't care about the failure
   */
  fd = open(file_name, O_RDWR);
  if (fd < 0) {
    LOG_ERROR("Failed to open for readwrite %s, due to %s.", file_name, strerror(errno));
    return RC::IOERR_ACCESS;
  }

  Page page;
  memset(&page, 0, sizeof(Page));

  BPFileHeader *file_header = (BPFileHeader *)page.data;
  file_header->allocated_pages = 1;
  file_header->page_count = 1;

  char *bitmap = file_header->bitmap;
  bitmap[0] |= 0x01;
  if (lseek(fd, 0, SEEK_SET) == -1) {
    LOG_ERROR("Failed to seek file %s to position 0, due to %s .", file_name, strerror(errno));
    close(fd);
    return RC::IOERR_SEEK;
  }

  if (writen(fd, (char *)&page, sizeof(Page)) != 0) {
    LOG_ERROR("Failed to write header to file %s, due to %s.", file_name, strerror(errno));
    close(fd);
    return RC::IOERR_WRITE;
  }

  close(fd);
  LOG_INFO("Successfully create %s.", file_name);
  return RC::SUCCESS;
}

RC BufferPoolManager::open_file(const char *_file_name, DiskBufferPool *& _bp)
{
  std::string file_name(_file_name);
  
  if (buffer_pools_.find(file_name) != buffer_pools_.end()) {
    LOG_WARN("file already opened. file name=%s", _file_name);
    return RC::BUFFERPOOL_OPEN;
  }

  DiskBufferPool *bp = new DiskBufferPool(*this, frame_manager_);
  RC rc = bp->open_file(_file_name);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to open file name");
    delete bp;
    return rc;
  }

  buffer_pools_.insert(std::pair<std::string, DiskBufferPool *>(file_name, bp));
  fd_buffer_pools_.insert(std::pair<int, DiskBufferPool *>(bp->file_desc(), bp));
  _bp = bp;
  return RC::SUCCESS;
}

RC BufferPoolManager::close_file(const char *_file_name)
{
  std::string file_name(_file_name);
  auto iter = buffer_pools_.find(file_name);
  if (iter == buffer_pools_.end()) {
    LOG_WARN("file has not opened: %s", _file_name);
    return RC::INTERNAL;
  }

  int fd = iter->second->file_desc();
  fd_buffer_pools_.erase(fd);

  DiskBufferPool *bp = iter->second;
  buffer_pools_.erase(iter);
  delete bp;
  return RC::SUCCESS;
}

RC BufferPoolManager::flush_page(Frame &frame)
{
  int fd = frame.file_desc();
  auto iter = fd_buffer_pools_.find(fd);
  if (iter == fd_buffer_pools_.end()) {
    LOG_WARN("unknown buffer pool of fd %d", fd);
    return RC::INTERNAL;
  }

  DiskBufferPool *bp = iter->second;
  return bp->flush_page(frame);
}

static BufferPoolManager *default_bpm = nullptr;
void BufferPoolManager::set_instance(BufferPoolManager *bpm)
{
  if (default_bpm != nullptr && bpm != nullptr) {
    LOG_ERROR("default buffer pool manager has been setted");
    abort();
  }
  default_bpm = bpm;
}
BufferPoolManager &BufferPoolManager::instance()
{
  return *default_bpm;
}
