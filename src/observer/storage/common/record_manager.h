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
#ifndef __OBSERVER_STORAGE_COMMON_RECORD_MANAGER_H_
#define __OBSERVER_STORAGE_COMMON_RECORD_MANAGER_H_

#include <sstream>
#include <limits>
#include "storage/default/disk_buffer_pool.h"
#include "common/lang/bitmap.h"

typedef int32_t SlotNum;

class ConditionFilter;

struct PageHeader {
  int record_num;           // 当前页面记录的个数
  int record_capacity;      // 最大记录个数
  int record_real_size;     // 每条记录的实际大小
  int record_size;          // 每条记录占用实际空间大小(可能对齐)
  int first_record_offset;  // 第一条记录的偏移量
};

struct RID {
  PageNum page_num;  // record's page number
  SlotNum slot_num;  // record's slot number
  // bool    valid;    // true means a valid record

  const std::string to_string() const
  {
    std::stringstream ss;

    ss << "PageNum:" << page_num << ", SlotNum:" << slot_num;

    return ss.str();
  }

  bool operator==(const RID &other) const
  {
    return page_num == other.page_num && slot_num == other.slot_num;
  }

  bool operator!=(const RID &other) const
  {
    return !(*this == other);
  }

  static int compare(const RID *rid1, const RID *rid2)
  {
    int page_diff = rid1->page_num - rid2->page_num;
    if (page_diff != 0) {
      return page_diff;
    } else {
      return rid1->slot_num - rid2->slot_num;
    }
  }

  /**
   * 返回一个不可能出现的最小的RID
   * 虽然page num 0和slot num 0都是合法的，但是page num 0通常用于存放meta数据，所以对数据部分来说都是
   * 不合法的. 这里在bplus tree中查找时会用到。
   */
  static RID *min()
  {
    static RID rid{0, 0};
    return &rid;
  }
  static RID *max()
  {
    static RID rid{std::numeric_limits<PageNum>::max(), std::numeric_limits<SlotNum>::max()};
    return &rid;
  }
};

class RidDigest {
public:
  size_t operator()(const RID &rid) const
  {
    return ((size_t)(rid.page_num) << 32) | rid.slot_num;
  }
};

struct Record {
  // bool valid; // false means the record hasn't been load
  RID rid;     // record's rid
  char *data;  // record's data
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
  DiskBufferPool *disk_buffer_pool_ = nullptr;
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
  RC fetch_record_in_page();
private:
  DiskBufferPool *disk_buffer_pool_ = nullptr;

  BufferPoolIterator bp_iterator_;
  ConditionFilter *condition_filter_ = nullptr;
  RecordPageHandler record_page_handler_;
  RecordPageIterator record_page_iterator_;
  Record next_record_;
};

#endif  //__OBSERVER_STORAGE_COMMON_RECORD_MANAGER_H_
