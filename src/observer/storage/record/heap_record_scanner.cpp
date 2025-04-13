/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "storage/record/heap_record_scanner.h"

////////////////////////////////////////////////////////////////////////////////


RC HeapRecordScanner::open_scan()
{
  ASSERT(disk_buffer_pool_ != nullptr, "disk buffer pool is null");
  ASSERT(log_handler_ != nullptr, "log handler is null");

  RC rc = bp_iterator_.init(*disk_buffer_pool_, 1);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to init bp iterator. rc=%d:%s", rc, strrc(rc));
    return rc;
  }
  if (table_ == nullptr || table_->table_meta().storage_format() == StorageFormat::ROW_FORMAT) {
    record_page_handler_ = new RowRecordPageHandler();
  } else {
    record_page_handler_ = new PaxRecordPageHandler();
  }

  return rc;
}

/**
 * @brief 从当前位置开始找到下一条有效的记录
 *
 * 如果当前页面还有记录没有访问，就遍历当前的页面。
 * 当前页面遍历完了，就遍历下一个页面，然后找到有效的记录
 */
RC HeapRecordScanner::fetch_next_record()
{
  RC rc = RC::SUCCESS;
  if (record_page_iterator_.is_valid()) {
    // 当前页面还是有效的，尝试看一下是否有有效记录
    rc = fetch_next_record_in_page();
    if (rc == RC::SUCCESS || rc != RC::RECORD_EOF) {
      // 有有效记录：RC::SUCCESS
      // 或者出现了错误，rc != (RC::SUCCESS or RC::RECORD_EOF)
      // RECORD_EOF 表示当前页面已经遍历完了
      return rc;
    }
  }

  // 上个页面遍历完了，或者还没有开始遍历某个页面，那么就从一个新的页面开始遍历查找
  while (bp_iterator_.has_next()) {
    PageNum page_num = bp_iterator_.next();
    record_page_handler_->cleanup();
    rc = record_page_handler_->init(*disk_buffer_pool_, *log_handler_, page_num, rw_mode_);
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to init record page handler. page_num=%d, rc=%s", page_num, strrc(rc));
      return rc;
    }

    record_page_iterator_.init(record_page_handler_);
    rc = fetch_next_record_in_page();
    if (rc == RC::SUCCESS || rc != RC::RECORD_EOF) {
      // 有有效记录：RC::SUCCESS
      // 或者出现了错误，rc != (RC::SUCCESS or RC::RECORD_EOF)
      // RECORD_EOF 表示当前页面已经遍历完了
      return rc;
    }
  }

  // 所有的页面都遍历完了，没有数据了
  next_record_.rid().slot_num = -1;
  record_page_handler_->cleanup();
  return RC::RECORD_EOF;
}

/**
 * @brief 遍历当前页面，尝试找到一条有效的记录
 */
RC HeapRecordScanner::fetch_next_record_in_page()
{
  RC rc = RC::SUCCESS;
  while (record_page_iterator_.has_next()) {
    rc = record_page_iterator_.next(next_record_);
    if (rc != RC::SUCCESS) {
      const auto page_num = record_page_handler_->get_page_num();
      LOG_TRACE("failed to get next record from page. page_num=%d, rc=%s", page_num, strrc(rc));
      return rc;
    }

    // 如果有过滤条件，就用过滤条件过滤一下
    if (condition_filter_ != nullptr && !condition_filter_->filter(next_record_)) {
      continue;
    }

    // 如果是某个事务上遍历数据，还要看看事务访问是否有冲突
    if (trx_ == nullptr) {
      return rc;
    }

    // 让当前事务探测一下是否访问冲突，或者需要加锁、等锁等操作，由事务自己决定
    // TODO 把判断事务有效性的逻辑从Scanner中移除
    rc = trx_->visit_record(table_, next_record_, rw_mode_);
    if (rc == RC::RECORD_INVISIBLE) {
      // 可以参考MvccTrx，表示当前记录不可见
      // 这种模式仅在 readonly 事务下是有效的
      continue;
    }
    return rc;
  }

  next_record_.rid().slot_num = -1;
  return RC::RECORD_EOF;
}

RC HeapRecordScanner::close_scan()
{
  if (disk_buffer_pool_ != nullptr) {
    disk_buffer_pool_ = nullptr;
  }

  if (condition_filter_ != nullptr) {
    condition_filter_ = nullptr;
  }
  if (record_page_handler_ != nullptr) {
    record_page_handler_->cleanup();
    delete record_page_handler_;
    record_page_handler_ = nullptr;
  }

  return RC::SUCCESS;
}

RC HeapRecordScanner::next(Record &record)
{
  RC rc = fetch_next_record();
  if (OB_FAIL(rc)) {
    return rc;
  }

  record = next_record_;
  return RC::SUCCESS;
}
