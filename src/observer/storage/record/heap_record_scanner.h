/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#pragma once

#include "storage/record/record_scanner.h"
#include "storage/buffer/disk_buffer_pool.h"
#include "storage/trx/trx.h"

/**
 * @brief 遍历某个文件中所有记录
 * @ingroup RecordManager
 * @details 遍历所有的页面，同时访问这些页面中所有的记录
 */
class HeapRecordScanner : public RecordScanner
{
public:
  HeapRecordScanner(Table *table, DiskBufferPool &buffer_pool, Trx *trx, LogHandler &log_handler, ReadWriteMode mode,
      ConditionFilter *condition_filter)
      : table_(table),
        disk_buffer_pool_(&buffer_pool),
        trx_(trx),
        log_handler_(&log_handler),
        rw_mode_(mode),
        condition_filter_(condition_filter)
  {}
  ~HeapRecordScanner() override { close_scan(); }

  /**
   * @brief 打开一个文件扫描。
   */
  RC open_scan() override;

  /**
   * @brief 关闭一个文件扫描，释放相应的资源
   */
  RC close_scan() override;

  /**
   * @brief 获取下一条记录
   *
   * @param record 返回的下一条记录
   */
  RC next(Record &record) override;

private:
  /**
   * @brief 获取该文件中的下一条记录
   */
  RC fetch_next_record();

  /**
   * @brief 获取一个页面内的下一条记录
   */
  RC fetch_next_record_in_page();

private:
  // TODO 对于一个纯粹的record遍历器来说，不应该关心表和事务
  Table *table_ = nullptr;  ///< 当前遍历的是哪张表。这个字段仅供事务函数使用，如果设计合适，可以去掉

  DiskBufferPool *disk_buffer_pool_ = nullptr;  ///< 当前访问的文件
  Trx            *trx_              = nullptr;  ///< 当前是哪个事务在遍历
  LogHandler     *log_handler_      = nullptr;
  ReadWriteMode   rw_mode_ = ReadWriteMode::READ_WRITE;  ///< 遍历出来的数据，是否可能对它做修改

  BufferPoolIterator bp_iterator_;                    ///< 遍历buffer pool的所有页面
  ConditionFilter   *condition_filter_    = nullptr;  ///< 过滤record
  RecordPageHandler *record_page_handler_ = nullptr;  ///< 处理文件某页面的记录
  RecordPageIterator record_page_iterator_;           ///< 遍历某个页面上的所有record
  Record             next_record_;                    ///< 获取的记录放在这里缓存起来
};