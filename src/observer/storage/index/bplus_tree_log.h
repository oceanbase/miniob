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
// Created by wangyunlai.wyl on 2024/02/05.
//

#pragma once

#include <stddef.h>
#include <string>
#include <vector>
#include <span>

#include "common/types.h"
#include "common/rc.h"
#include "storage/index/latch_memo.h"
// #include "storage/index/bplus_tree_log_entry.h"

class LogHandler;
class BplusTreeHandler;
class Frame;
class IndexNodeHandler;
class BplusTreeMiniTransaction;

namespace bplus_tree {
class LogEntry;
}

class BplusTreeLogger final
{
public:
  BplusTreeLogger(LogHandler &log_handler, int32_t buffer_pool_id);
  ~BplusTreeLogger();

  RC init_header_page(
      Frame *frame, int32_t attr_length, int32_t attr_type, int32_t internal_max_size, int32_t leaf_max_size);
  RC update_root_page(Frame *frame, PageNum root_page_num, PageNum old_page_num);

  RC node_insert_items(IndexNodeHandler &node_handler, int index, std::span<const char> items, int item_num);
  RC node_remove_items(IndexNodeHandler &node_handler, int index, std::span<const char> items, int item_num);

  RC leaf_init_empty(IndexNodeHandler &node_handler);
  RC leaf_set_next_page(IndexNodeHandler &node_handler, PageNum page_num, PageNum old_page_num);

  RC internal_init_empty(IndexNodeHandler &node_handler);
  RC internal_create_new_root(
      IndexNodeHandler &node_handler, PageNum first_page_num, std::span<const char> key, PageNum page_num);
  RC internal_update_key(
      IndexNodeHandler &node_handler, int index, std::span<const char> key, std::span<const char> old_key);

  RC set_parent_page(IndexNodeHandler &node_handler, PageNum page_num, PageNum old_page_num);

  RC commit();
  RC rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler);

protected:
  RC append_log_entry(std::unique_ptr<bplus_tree::LogEntry> entry);

private:
  LogHandler &log_handler_;
  int32_t     buffer_pool_id_ = -1;

  std::vector<std::unique_ptr<bplus_tree::LogEntry>> entries_;

  bool rolling_back_ = false;
};

class BplusTreeMiniTransaction
{
public:
  BplusTreeMiniTransaction(BplusTreeHandler &tree_handler, RC *operation_result = nullptr);
  ~BplusTreeMiniTransaction();

  LatchMemo       &latch_memo() { return latch_memo_; }
  BplusTreeLogger &logger() { return logger_; }

  RC commit();
  RC rollback();

private:
  BplusTreeHandler &tree_handler_;
  RC               *operation_result_ = nullptr;
  LatchMemo       latch_memo_;
  BplusTreeLogger logger_;
};