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
#pragma once

#include <sstream>
#include <limits>
#include "storage/default/disk_buffer_pool.h"
#include "storage/record/record.h"
#include "common/lang/bitmap.h"

class ConditionFilter;

struct PageHeader {
  int32_t record_num;           // 当前页面记录的个数
  int32_t record_capacity;      // 最大记录个数
  int32_t record_real_size;     // 每条记录的实际大小
  int32_t record_size;          // 每条记录占用实际空间大小(可能对齐)
  int32_t first_record_offset;  // 第一条记录的偏移量
};

class RecordPageHandler;
class RecordPageIterator
{
public:
  RecordPageIterator();
  ~RecordPageIterator();

  void init(RecordPageHandler &record_page_handler);

  bool has_next();
  RC   next(Record &record);

  bool is_valid() const {
    return record_page_handler_ != nullptr;
  }
private:
  RecordPageHandler *record_page_handler_ = nullptr;
  PageNum page_num_ = BP_INVALID_PAGE_NUM;
  common::Bitmap  bitmap_;
  SlotNum next_slot_num_ = 0;
};

class RecordPageHandler {
public:
  RecordPageHandler() = default;
  ~RecordPageHandler();
  RC init(DiskBufferPool &buffer_pool, PageNum page_num);
  RC init_empty_page(DiskBufferPool &buffer_pool, PageNum page_num, int record_size);
  RC cleanup();

  RC insert_record(const char *data, RID *rid);
  RC update_record(const Record *rec);

  template <class RecordUpdater>
  RC update_record_in_place(const RID *rid, RecordUpdater updater)
  {
    Record record;
    RC rc = get_record(rid, &record);
    if (rc != RC::SUCCESS) {
      return rc;
    }
    rc = updater(record);
    frame_->mark_dirty();
    return rc;
  }

  RC delete_record(const RID *rid);

  RC get_record(const RID *rid, Record *rec);

  PageNum get_page_num() const;

  bool is_full() const;

protected:
  char *get_record_data(SlotNum slot_num)
  {
    return frame_->data() + page_header_->first_record_offset + (page_header_->record_size * slot_num);
  }

protected:
  DiskBufferPool *disk_buffer_pool_ = nullptr;
  Frame *frame_ = nullptr;
  PageHeader *page_header_ = nullptr;
  char *bitmap_ = nullptr;

private:
  friend class RecordPageIterator;
};

class RecordFileHandler {
public:
  RecordFileHandler() = default;
  RC init(DiskBufferPool *buffer_pool);
  void close();

  /**
   * 更新指定文件中的记录，rec指向的记录结构中的rid字段为要更新的记录的标识符，
   * pData字段指向新的记录内容
   */
  RC update_record(const Record *rec);

  /**
   * 从指定文件中删除标识符为rid的记录
   */
  RC delete_record(const RID *rid);

  /**
   * 插入一个新的记录到指定文件中，pData为指向新纪录内容的指针，返回该记录的标识符rid
   */
  RC insert_record(const char *data, int record_size, RID *rid);

  /**
   * 获取指定文件中标识符为rid的记录内容到rec指向的记录结构中
   */
  RC get_record(const RID *rid, Record *rec);

  template <class RecordUpdater>  // 改成普通模式, 不使用模板
  RC update_record_in_place(const RID *rid, RecordUpdater updater)
  {

    RC rc = RC::SUCCESS;
    RecordPageHandler page_handler;
    if ((rc != page_handler.init(*disk_buffer_pool_, rid->page_num)) != RC::SUCCESS) {
      return rc;
    }

    return page_handler.update_record_in_place(rid, updater);
  }

private:
  RC init_free_pages();
  
private:
  DiskBufferPool *disk_buffer_pool_ = nullptr;
  std::unordered_set<PageNum>  free_pages_; // 没有填充满的页面集合
};

class RecordFileScanner {
public:
  RecordFileScanner() = default;

  /**
   * 打开一个文件扫描。
   * 如果条件不为空，则要对每条记录进行条件比较，只有满足所有条件的记录才被返回
   */
  RC open_scan(DiskBufferPool &buffer_pool, ConditionFilter *condition_filter);

  /**
   * 关闭一个文件扫描，释放相应的资源
   */
  RC close_scan();

  bool has_next();
  RC   next(Record &record);

private:
  RC fetch_next_record();
  RC fetch_next_record_in_page();
private:
  DiskBufferPool *disk_buffer_pool_ = nullptr;

  BufferPoolIterator bp_iterator_;
  ConditionFilter *condition_filter_ = nullptr;
  RecordPageHandler record_page_handler_;
  RecordPageIterator record_page_iterator_;
  Record next_record_;
};
