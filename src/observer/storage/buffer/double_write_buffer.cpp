/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Wenbin1002 on 2024/04/16
//
#include <fcntl.h>

#include <mutex>
#include <algorithm>

#include "storage/buffer/double_write_buffer.h"
#include "storage/buffer/disk_buffer_pool.h"
#include "common/io/io.h"
#include "common/log/log.h"
#include "common/math/crc.h"

using namespace common;

struct DoubleWritePage
{
public:
  DoubleWritePage() = default;
  DoubleWritePage(int32_t buffer_pool_id, PageNum page_num, int32_t page_index, Page &page);

public:
  DoubleWritePageKey key;
  int32_t            page_index = -1; /// 页面在double write buffer文件中的页索引
  bool               valid = true; /// 表示页面是否有效，在页面被删除时，需要同时标记磁盘上的值。
  Page               page;

  static const int32_t SIZE;
};

DoubleWritePage::DoubleWritePage(int32_t buffer_pool_id, PageNum page_num, int32_t page_index, Page &_page)
  : key{buffer_pool_id, page_num}, page_index(page_index), page(_page)
{}

const int32_t DoubleWritePage::SIZE = sizeof(DoubleWritePage);

const int32_t DoubleWriteBufferHeader::SIZE = sizeof(DoubleWriteBufferHeader);

DiskDoubleWriteBuffer::DiskDoubleWriteBuffer(BufferPoolManager &bp_manager, int max_pages /*=16*/) 
  : max_pages_(max_pages), bp_manager_(bp_manager)
{
}

DiskDoubleWriteBuffer::~DiskDoubleWriteBuffer()
{
  flush_page();
  close(file_desc_);
}

RC DiskDoubleWriteBuffer::open_file(const char *filename)
{
  if (file_desc_ >= 0) {
    LOG_ERROR("Double write buffer has already opened. file desc=%d", file_desc_);
    return RC::BUFFERPOOL_OPEN;
  }
  
  int fd = open(filename, O_CREAT | O_RDWR, 0644);
  if (fd < 0) {
    LOG_ERROR("Failed to open or creat %s, due to %s.", filename, strerror(errno));
    return RC::SCHEMA_DB_EXIST;
  }

  file_desc_ = fd;
  return load_pages();
}

RC DiskDoubleWriteBuffer::flush_page()
{
  sync();

  for (const auto &pair : dblwr_pages_) {
    RC rc = write_page(pair.second);
    if (rc != RC::SUCCESS) {
      return rc;
    }
    pair.second->valid = false;
    write_page_internal(pair.second);
    delete pair.second;
  }

  dblwr_pages_.clear();
  header_.page_cnt = 0;

  return RC::SUCCESS;
}

RC DiskDoubleWriteBuffer::add_page(DiskBufferPool *bp, PageNum page_num, Page &page)
{
  scoped_lock lock_guard(lock_);
  DoubleWritePageKey key{bp->id(), page_num};
  auto iter = dblwr_pages_.find(key);
  if (iter != dblwr_pages_.end()) {
    iter->second->page = page;
    LOG_TRACE("[cache hit]add page into double write buffer. buffer_pool_id:%d,page_num:%d,lsn=%d, dwb size=%d",
              bp->id(), page_num, page.lsn, static_cast<int>(dblwr_pages_.size()));
    return write_page_internal(iter->second);
  }

  int64_t          page_cnt   = dblwr_pages_.size();
  DoubleWritePage *dblwr_page = new DoubleWritePage(bp->id(), page_num, page_cnt, page);
  dblwr_pages_.insert(std::pair<DoubleWritePageKey, DoubleWritePage *>(key, dblwr_page));
  LOG_TRACE("insert page into double write buffer. buffer_pool_id:%d,page_num:%d,lsn=%d, dwb size:%d",
            bp->id(), page_num, page.lsn, static_cast<int>(dblwr_pages_.size()));

  RC rc = write_page_internal(dblwr_page);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to write page into double write buffer. rc=%s buffer_pool_id:%d,page_num:%d,lsn=%d.",
        strrc(rc), bp->id(), page_num, page.lsn);
    return rc;
  }

  if (page_cnt + 1 > header_.page_cnt) {
    header_.page_cnt = page_cnt + 1;
    if (lseek(file_desc_, 0, SEEK_SET) == -1) {
      LOG_ERROR("Failed to add page header due to failed to seek %s.", strerror(errno));
      return RC::IOERR_SEEK;
    }

    if (writen(file_desc_, &header_, sizeof(header_)) != 0) {
      LOG_ERROR("Failed to add page header due to %s.", strerror(errno));
      return RC::IOERR_WRITE;
    }
  }

  if (static_cast<int>(dblwr_pages_.size()) >= max_pages_) {
    RC rc = flush_page();
    if (rc != RC::SUCCESS) {
      LOG_ERROR("Failed to flush pages in double write buffer");
      return rc;
    }
  }

  return RC::SUCCESS;
}

RC DiskDoubleWriteBuffer::write_page_internal(DoubleWritePage *page)
{
  int32_t page_index = page->page_index;
  int64_t offset = page_index * DoubleWritePage::SIZE + DoubleWriteBufferHeader::SIZE;
  if (lseek(file_desc_, offset, SEEK_SET) == -1) {
    LOG_ERROR("Failed to add page %lld of %d due to failed to seek %s.", offset, file_desc_, strerror(errno));
    return RC::IOERR_SEEK;
  }

  if (writen(file_desc_, page, DoubleWritePage::SIZE) != 0) {
    LOG_ERROR("Failed to add page %lld of %d due to %s.", offset, file_desc_, strerror(errno));
    return RC::IOERR_WRITE;
  }

  return RC::SUCCESS;
}

RC DiskDoubleWriteBuffer::write_page(DoubleWritePage *dblwr_page)
{
  DiskBufferPool *disk_buffer = nullptr;
  // skip invalid page
  if (!dblwr_page->valid) {
    LOG_TRACE("double write buffer write page invalid. buffer_pool_id:%d,page_num:%d,lsn=%d",
              dblwr_page->key.buffer_pool_id, dblwr_page->key.page_num, dblwr_page->page.lsn);
    return RC::SUCCESS;
  }
  RC rc = bp_manager_.get_buffer_pool(dblwr_page->key.buffer_pool_id, disk_buffer);
  ASSERT(OB_SUCC(rc) && disk_buffer != nullptr, "failed to get disk buffer pool of %d", dblwr_page->key.buffer_pool_id);

  LOG_TRACE("double write buffer write page. buffer_pool_id:%d,page_num:%d,lsn=%d",
            dblwr_page->key.buffer_pool_id, dblwr_page->key.page_num, dblwr_page->page.lsn);

  return disk_buffer->write_page(dblwr_page->key.page_num, dblwr_page->page);
}

RC DiskDoubleWriteBuffer::read_page(DiskBufferPool *bp, PageNum page_num, Page &page)
{
  scoped_lock lock_guard(lock_);
  DoubleWritePageKey key{bp->id(), page_num};
  auto iter = dblwr_pages_.find(key);
  if (iter != dblwr_pages_.end()) {
    page = iter->second->page;
    LOG_TRACE("double write buffer read page success. bp id=%d, page_num:%d, lsn:%d", bp->id(), page_num, page.lsn);
    return RC::SUCCESS;
  }

  return RC::BUFFERPOOL_INVALID_PAGE_NUM;
}

RC DiskDoubleWriteBuffer::clear_pages(DiskBufferPool *buffer_pool)
{
  vector<DoubleWritePage *> spec_pages;
  
  auto remove_pred = [&spec_pages, buffer_pool](const pair<DoubleWritePageKey, DoubleWritePage *> &pair) {
    DoubleWritePage *dbl_page = pair.second;
    if (buffer_pool->id() == dbl_page->key.buffer_pool_id) {
      spec_pages.push_back(dbl_page);
      return true;
    }
    return false;
  };

  lock_.lock();
  erase_if(dblwr_pages_, remove_pred);
  lock_.unlock();

  LOG_INFO("clear pages in double write buffer. file name=%s, page count=%d",
           buffer_pool->filename(), spec_pages.size());

  // 页面从小到大排序，防止出现小页面还没有写入，而页面编号更大的seek失败的情况
  sort(spec_pages.begin(), spec_pages.end(), [](DoubleWritePage *a, DoubleWritePage *b) {
    return a->key.page_num < b->key.page_num;
  });

  RC rc = RC::SUCCESS;
  for (DoubleWritePage *dbl_page : spec_pages) {
    rc = buffer_pool->write_page(dbl_page->key.page_num, dbl_page->page);
    if (OB_FAIL(rc)) {
      LOG_WARN("Failed to write page %s:%d to disk buffer pool. rc=%s",
               buffer_pool->filename(), dbl_page->key.page_num, strrc(rc));
      break;
    }
    dbl_page->valid = false;
    write_page_internal(dbl_page);
  }

  for_each(spec_pages.begin(), spec_pages.end(), [](DoubleWritePage *dbl_page) { delete dbl_page; });

  return RC::SUCCESS;
}

RC DiskDoubleWriteBuffer::load_pages()
{
  if (file_desc_ < 0) {
    LOG_ERROR("Failed to load pages, due to file desc is invalid.");
    return RC::BUFFERPOOL_OPEN;
  }

  if (!dblwr_pages_.empty()) {
    LOG_ERROR("Failed to load pages, due to double write buffer is not empty. opened?");
    return RC::BUFFERPOOL_OPEN;
  }

  if (lseek(file_desc_, 0, SEEK_SET) == -1) {
    LOG_ERROR("Failed to load page header, due to failed to lseek:%s.", strerror(errno));
    return RC::IOERR_SEEK;
  }

  int ret = readn(file_desc_, &header_, sizeof(header_));
  if (ret != 0 && ret != -1) {
    LOG_ERROR("Failed to load page header, file_desc:%d, due to failed to read data:%s, ret=%d",
                file_desc_, strerror(errno), ret);
    return RC::IOERR_READ;
  }

  for (int page_num = 0; page_num < header_.page_cnt; page_num++) {
    int64_t offset = ((int64_t)page_num) * DoubleWritePage::SIZE + DoubleWriteBufferHeader::SIZE;

    if (lseek(file_desc_, offset, SEEK_SET) == -1) {
      LOG_ERROR("Failed to load page %d, offset=%ld, due to failed to lseek:%s.", page_num, offset, strerror(errno));
      return RC::IOERR_SEEK;
    }

    auto dblwr_page = make_unique<DoubleWritePage>();
    Page &page     = dblwr_page->page;
    page.check_sum = (CheckSum)-1;

    ret = readn(file_desc_, dblwr_page.get(), DoubleWritePage::SIZE);
    if (ret != 0) {
      LOG_ERROR("Failed to load page, file_desc:%d, page num:%d, due to failed to read data:%s, ret=%d, page count=%d",
                file_desc_, page_num, strerror(errno), ret, page_num);
      return RC::IOERR_READ;
    }

    const CheckSum check_sum = crc32(page.data, BP_PAGE_DATA_SIZE);
    if (check_sum == page.check_sum) {
      DoubleWritePageKey key = dblwr_page->key;
      dblwr_pages_.insert(pair<DoubleWritePageKey, DoubleWritePage *>(key, dblwr_page.release()));
    } else {
      LOG_TRACE("got a page with an invalid checksum. on disk:%d, in memory:%d", page.check_sum, check_sum);
    }
  }

  LOG_INFO("double write buffer load pages done. page num=%d", dblwr_pages_.size());
  return RC::SUCCESS;
}

RC DiskDoubleWriteBuffer::recover()
{
  return flush_page();
}

////////////////////////////////////////////////////////////////
RC VacuousDoubleWriteBuffer::add_page(DiskBufferPool *bp, PageNum page_num, Page &page)
{
  return bp->write_page(page_num, page);
}

