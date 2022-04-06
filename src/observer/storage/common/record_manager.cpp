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
#include "storage/common/record_manager.h"
#include "rc.h"
#include "common/log/log.h"
#include "common/lang/bitmap.h"
#include "condition_filter.h"

using namespace common;

int align8(int size)
{
  return size / 8 * 8 + ((size % 8 == 0) ? 0 : 8);
}

int page_fix_size()
{
  return sizeof(PageHeader);
}

int page_record_capacity(int page_size, int record_size)
{
  // (record_capacity * record_size) + record_capacity/8 + 1 <= (page_size - fix_size)
  // ==> record_capacity = ((page_size - fix_size) - 1) / (record_size + 0.125)
  return (int)((page_size - page_fix_size() - 1) / (record_size + 0.125));
}

int page_bitmap_size(int record_capacity)
{
  return record_capacity / 8 + ((record_capacity % 8 == 0) ? 0 : 1);
}

int page_header_size(int record_capacity)
{
  const int bitmap_size = page_bitmap_size(record_capacity);
  return align8(page_fix_size() + bitmap_size);
}
////////////////////////////////////////////////////////////////////////////////

RecordPageHandler::~RecordPageHandler()
{
  cleanup();
}

RC RecordPageHandler::init(DiskBufferPool &buffer_pool, PageNum page_num)
{
  if (disk_buffer_pool_ != nullptr) {
    LOG_WARN("Disk buffer pool has been opened for page_num %d.", page_num);
    return RC::RECORD_OPENNED;
  }

  RC ret = RC::SUCCESS;
  if ((ret = buffer_pool.get_this_page(page_num, &frame_)) != RC::SUCCESS) {
    LOG_ERROR("Failed to get page handle from disk buffer pool. ret=%d:%s", ret, strrc(ret));
    return ret;
  }

  char *data = frame_->data();

  disk_buffer_pool_ = &buffer_pool;

  page_header_ = (PageHeader *)(data);
  bitmap_ = data + page_fix_size();
  LOG_TRACE("Successfully init page_num %d.", page_num);
  return ret;
}

RC RecordPageHandler::init_empty_page(DiskBufferPool &buffer_pool, PageNum page_num, int record_size)
{
  RC ret = init(buffer_pool, page_num);
  if (ret != RC::SUCCESS) {
    LOG_ERROR("Failed to init empty page page_num:record_size %d:%d.", page_num, record_size);
    return ret;
  }

  int page_size = BP_PAGE_DATA_SIZE;
  int record_phy_size = align8(record_size);
  page_header_->record_num = 0;
  page_header_->record_capacity = page_record_capacity(page_size, record_phy_size);
  page_header_->record_real_size = record_size;
  page_header_->record_size = record_phy_size;
  page_header_->first_record_offset = page_header_size(page_header_->record_capacity);
  bitmap_ = frame_->data() + page_fix_size();

  memset(bitmap_, 0, page_bitmap_size(page_header_->record_capacity));
  frame_->mark_dirty();

  return RC::SUCCESS;
}

RC RecordPageHandler::cleanup()
{
  if (disk_buffer_pool_ != nullptr) {
    disk_buffer_pool_->unpin_page(frame_);
    disk_buffer_pool_ = nullptr;
  }

  return RC::SUCCESS;
}

RC RecordPageHandler::insert_record(const char *data, RID *rid)
{
  if (page_header_->record_num == page_header_->record_capacity) {
    LOG_WARN("Page is full, page_num %d:%d.", frame_->page_num());
    return RC::RECORD_NOMEM;
  }

  // 找到空闲位置
  Bitmap bitmap(bitmap_, page_header_->record_capacity);
  int index = bitmap.next_unsetted_bit(0);
  bitmap.set_bit(index);
  page_header_->record_num++;

  // assert index < page_header_->record_capacity
  char *record_data = get_record_data(index);
  memcpy(record_data, data, page_header_->record_real_size);

  frame_->mark_dirty();

  if (rid) {
    rid->page_num = get_page_num();
    rid->slot_num = index;
  }

  // LOG_TRACE("Insert record. rid page_num=%d, slot num=%d", get_page_num(), index);
  return RC::SUCCESS;
}

RC RecordPageHandler::update_record(const Record *rec)
{
  if (rec->rid.slot_num >= page_header_->record_capacity) {
    LOG_ERROR("Invalid slot_num %d, exceed page's record capacity, page_num %d.",
	      rec->rid.slot_num, frame_->page_num());
    return RC::INVALID_ARGUMENT;
  }

  Bitmap bitmap(bitmap_, page_header_->record_capacity);
  if (!bitmap.get_bit(rec->rid.slot_num)) {
    LOG_ERROR("Invalid slot_num %d, slot is empty, page_num %d.",
	      rec->rid.slot_num, frame_->page_num());
    return RC::RECORD_RECORD_NOT_EXIST;
  } else {
    char *record_data = get_record_data(rec->rid.slot_num);
    memcpy(record_data, rec->data, page_header_->record_real_size);
    bitmap.set_bit(rec->rid.slot_num);
    frame_->mark_dirty();
    // LOG_TRACE("Update record. file_id=%d, page num=%d,slot=%d", file_id_, rec->rid.page_num, rec->rid.slot_num);
    return RC::SUCCESS;
  }
}

RC RecordPageHandler::delete_record(const RID *rid)
{
  if (rid->slot_num >= page_header_->record_capacity) {
    LOG_ERROR("Invalid slot_num %d, exceed page's record capacity, page_num %d.",
	      rid->slot_num, frame_->page_num());
    return RC::INVALID_ARGUMENT;
  }

  Bitmap bitmap(bitmap_, page_header_->record_capacity);
  if (bitmap.get_bit(rid->slot_num)) {
    bitmap.clear_bit(rid->slot_num);
    page_header_->record_num--;
    frame_->mark_dirty();

    if (page_header_->record_num == 0) {
      DiskBufferPool *disk_buffer_pool = disk_buffer_pool_;
      PageNum page_num = get_page_num();
      cleanup();
      disk_buffer_pool->dispose_page(page_num);
    }
    return RC::SUCCESS;
  } else {
    LOG_ERROR("Invalid slot_num %d, slot is empty, page_num %d.",
	      rid->slot_num, frame_->page_num());
    return RC::RECORD_RECORD_NOT_EXIST;
  }
}

RC RecordPageHandler::get_record(const RID *rid, Record *rec)
{
  if (rid->slot_num >= page_header_->record_capacity) {
    LOG_ERROR("Invalid slot_num:%d, exceed page's record capacity, page_num %d.",
	      rid->slot_num, frame_->page_num());
    return RC::RECORD_INVALIDRID;
  }

  Bitmap bitmap(bitmap_, page_header_->record_capacity);
  if (!bitmap.get_bit(rid->slot_num)) {
    LOG_ERROR("Invalid slot_num:%d, slot is empty, page_num %d.",
	      rid->slot_num, frame_->page_num());
    return RC::RECORD_RECORD_NOT_EXIST;
  }

  rec->rid = *rid;
  rec->data = get_record_data(rid->slot_num);
  return RC::SUCCESS;
}

RC RecordPageHandler::get_first_record(Record *rec)
{
  rec->rid.slot_num = -1;
  return get_next_record(rec);
}

RC RecordPageHandler::get_next_record(Record *rec)
{
  if (rec->rid.slot_num >= page_header_->record_capacity - 1) {
    LOG_ERROR("Invalid slot_num:%d, exceed page's record capacity, page_num %d.",
	      rec->rid.slot_num, frame_->page_num());
    return RC::RECORD_EOF;
  }

  Bitmap bitmap(bitmap_, page_header_->record_capacity);
  int index = bitmap.next_setted_bit(rec->rid.slot_num + 1);

  if (index < 0) {
    LOG_WARN("There is no empty slot on page -- page_num:%d.", frame_->page_num());
    return RC::RECORD_EOF;
  }

  rec->rid.page_num = get_page_num();
  rec->rid.slot_num = index;
  rec->data = get_record_data(index);
  return RC::SUCCESS;
}

PageNum RecordPageHandler::get_page_num() const
{
  if (nullptr == page_header_) {
    return (PageNum)(-1);
  }
  return frame_->page_num();
}

bool RecordPageHandler::is_full() const
{
  return page_header_->record_num >= page_header_->record_capacity;
}

////////////////////////////////////////////////////////////////////////////////

RC RecordFileHandler::init(DiskBufferPool *buffer_pool)
{
  if (disk_buffer_pool_ != nullptr) {
    LOG_ERROR("record file handler has been openned.");
    return RC::RECORD_OPENNED;
  }

  disk_buffer_pool_ = buffer_pool;

  LOG_INFO("Successfully open record file handle");
  return RC::SUCCESS;
}

void RecordFileHandler::close()
{
  if (disk_buffer_pool_ != nullptr) {
    disk_buffer_pool_ = nullptr;
  }
}

RC RecordFileHandler::insert_record(const char *data, int record_size, RID *rid)
{
  RC ret = RC::SUCCESS;
  // 找到没有填满的页面
  int page_count = 0;
  if ((ret = disk_buffer_pool_->get_page_count(&page_count)) != RC::SUCCESS) {
    LOG_ERROR("Failed to get page count while inserting record");
    return ret;
  }

  PageNum current_page_num = record_page_handler_.get_page_num();
  if (current_page_num < 0) {
    if (page_count >= 2) {
      // 当前buffer pool 有页面时才尝试加载第一页
      // 参考diskBufferPool，有效page的pageNum从1开始
      if ((ret = record_page_handler_.init(*disk_buffer_pool_, 1)) != RC::SUCCESS) {
        LOG_ERROR("Failed to init record page handler, ret=%d", ret);
        return ret;
      }
      current_page_num = record_page_handler_.get_page_num();
    } else {
      current_page_num = 0;
    }
  }

  bool page_found = false;
  for (int i = 0; i < page_count; i++) {
    current_page_num = (current_page_num + i) % page_count;  // 从当前打开的页面开始查找
    if (current_page_num == 0) {
      continue;
    }
    if (current_page_num != record_page_handler_.get_page_num()) {
      record_page_handler_.cleanup();
      ret = record_page_handler_.init(*disk_buffer_pool_, current_page_num);
      if (ret != RC::SUCCESS && ret != RC::BUFFERPOOL_INVALID_PAGE_NUM) {
        LOG_ERROR("Failed to init record page handler. page number is %d. ret=%d:%s",
		  current_page_num, ret, strrc(ret));
        return ret;
      }
    }

    if (!record_page_handler_.is_full()) {
      page_found = true;
      break;
    }
  }

  // 找不到就分配一个新的页面
  if (!page_found) {
    Frame *frame = nullptr;
    if ((ret = disk_buffer_pool_->allocate_page(&frame)) != RC::SUCCESS) {
      LOG_ERROR("Failed to allocate page while inserting record. ret:%d", ret);
      return ret;
    }

    current_page_num = frame->page_num();
    record_page_handler_.cleanup();
    ret = record_page_handler_.init_empty_page(*disk_buffer_pool_, current_page_num, record_size);
    if (ret != RC::SUCCESS) {
      LOG_ERROR("Failed to init empty page. ret:%d", ret);
      if (RC::SUCCESS != disk_buffer_pool_->unpin_page(frame)) {
        LOG_ERROR("Failed to unpin page. ");
      }
      return ret;
    }

    disk_buffer_pool_->unpin_page(frame);
  }

  // 找到空闲位置
  return record_page_handler_.insert_record(data, rid);
}

RC RecordFileHandler::update_record(const Record *rec)
{

  if (record_page_handler_.get_page_num() == rec->rid.page_num) {
    return record_page_handler_.update_record(rec);
  }
  RC ret;
  RecordPageHandler page_handler;
  if ((ret = page_handler.init(*disk_buffer_pool_, rec->rid.page_num)) != RC::SUCCESS) {
    LOG_ERROR("Failed to init record page handler.page number=%d", rec->rid.page_num);
    return ret;
  }

  return page_handler.update_record(rec);
}

RC RecordFileHandler::delete_record(const RID *rid)
{
  if (record_page_handler_.get_page_num() == rid->page_num) {
    return record_page_handler_.delete_record(rid);
  }

  RC ret = RC::SUCCESS;
  RecordPageHandler page_handler;
  if ((ret != page_handler.init(*disk_buffer_pool_, rid->page_num)) != RC::SUCCESS) {
    LOG_ERROR("Failed to init record page handler.page number=%d", rid->page_num);
    return ret;
  }
  return page_handler.delete_record(rid);
}

RC RecordFileHandler::get_record(const RID *rid, Record *rec)
{
  // lock?
  RC ret = RC::SUCCESS;
  if (nullptr == rid || nullptr == rec) {
    LOG_ERROR("Invalid rid %p or rec %p, one of them is null. ", rid, rec);
    return RC::INVALID_ARGUMENT;
  }

  if (record_page_handler_.get_page_num() == rid->page_num) {
    return record_page_handler_.get_record(rid, rec);
  }

  RecordPageHandler page_handler;
  if ((ret != page_handler.init(*disk_buffer_pool_, rid->page_num)) != RC::SUCCESS) {
    LOG_ERROR("Failed to init record page handler.page number=%d", rid->page_num);
    return ret;
  }

  return page_handler.get_record(rid, rec);
}

////////////////////////////////////////////////////////////////////////////////

RC RecordFileScanner::open_scan(DiskBufferPool &buffer_pool, ConditionFilter *condition_filter)
{
  close_scan();

  disk_buffer_pool_ = &buffer_pool;

  condition_filter_ = condition_filter;
  return RC::SUCCESS;
}

RC RecordFileScanner::close_scan()
{
  if (disk_buffer_pool_ != nullptr) {
    disk_buffer_pool_ = nullptr;
  }

  if (condition_filter_ != nullptr) {
    condition_filter_ = nullptr;
  }

  return RC::SUCCESS;
}

RC RecordFileScanner::get_first_record(Record *rec)
{
  rec->rid.page_num = 1;  // from 1 参考DiskBufferPool
  rec->rid.slot_num = -1;
  // rec->valid = false;
  return get_next_record(rec);
}

RC RecordFileScanner::get_next_record(Record *rec)
{
  if (nullptr == disk_buffer_pool_) {
    LOG_ERROR("Scanner has been closed.");
    return RC::RECORD_CLOSED;
  }

  RC ret = RC::SUCCESS;
  Record current_record = *rec;

  int page_count = 0;
  if ((ret = disk_buffer_pool_->get_page_count(&page_count)) != RC::SUCCESS) {
    LOG_ERROR("Failed to get page count while getting next record.");
    return RC::RECORD_EOF;
  }

  if (1 == page_count) {
    return RC::RECORD_EOF;
  }

  while (current_record.rid.page_num < page_count) {

    if (current_record.rid.page_num != record_page_handler_.get_page_num()) {
      record_page_handler_.cleanup();
      ret = record_page_handler_.init(*disk_buffer_pool_, current_record.rid.page_num);
      if (ret != RC::SUCCESS && ret != RC::BUFFERPOOL_INVALID_PAGE_NUM) {
        LOG_ERROR("Failed to init record page handler. page num=%d", current_record.rid.page_num);
        return ret;
      }

      if (RC::BUFFERPOOL_INVALID_PAGE_NUM == ret) {
        current_record.rid.page_num++;
        current_record.rid.slot_num = -1;
        continue;
      }
    }

    ret = record_page_handler_.get_next_record(&current_record);
    if (RC::SUCCESS == ret) {
      if (condition_filter_ == nullptr || condition_filter_->filter(current_record)) {
        break;  // got one
      }
    } else if (RC::RECORD_EOF == ret) {
      current_record.rid.page_num++;
      current_record.rid.slot_num = -1;
    } else {
      break;  // ERROR
    }
  }

  if (RC::SUCCESS == ret) {
    *rec = current_record;
  }
  return ret;
}
