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

#include <string>

#include "common/types.h"
#include "common/rc.h"
#include "storage/index/latch_memo.h"

class LogHandler;
class BplusTreeHandler;

class BplusTreeLogOperation
{
public:
  enum class Type
  {
    INIT_HEADER_PAGE,
  };

public:
  BplusTreeLogOperation(Type type) : type_(type) {}
  BplusTreeLogOperation(int type) : type_(static_cast<Type>(type)) {}

  Type type() const { return type_; }
  int  index() const { return static_cast<int>(type_); }

  std::string to_string() const;

private:
  Type type_;
};

struct BplusTreeLogHeader
{
  int32_t buffer_pool_id;
};

struct BplusTreeLogInitHeaderPageEntry
{
  PageNum page_num;
  int32_t attr_length;
  int32_t attr_type;
  int32_t internal_max_size;
  int32_t leaf_max_size;
};

struct BplusTreeLogEntry
{
  int32_t operation_type;
  PageNum page_num;
  char    data[0];
};

class BplusTreeLogger final
{
public:
  BplusTreeLogger(LogHandler &log_handler, int32_t buffer_pool_id, int32_t key_size, int32_t value_size);
  ~BplusTreeLogger();

  // RC init_header_page(PageNum page_num, int32_t attr_length, int32_t attr_type, int32_t internal_max_size, int32_t leaf_max_size);
  //RC leaf_insert_item(int index, const char *key, const char *value);
  //RC leaf_remove_item(int index);
  //RC leaf_copy(int start_index, const char *data, int length);
  //RC leaf_shink(int removed_item_num);
private:
  LogHandler &log_handler_;
  int32_t buffer_pool_id_ = -1;
  std::vector<BplusTreeLogEntry> entries_;
  int32_t key_size_ = -1; // 可能用不到，为了编译先加着
  int32_t value_size_ = -1;
};

class BplusTreeMiniTransaction
{
public:
  BplusTreeMiniTransaction(BplusTreeHandler &tree_handler);
  BplusTreeMiniTransaction(DiskBufferPool &buffer_pool, LogHandler &log_handler, int32_t key_size, int32_t value_size);
  ~BplusTreeMiniTransaction();

  LatchMemo &latch_memo() { return latch_memo_; }
  BplusTreeLogger &logger() { return logger_; }

private:
  LatchMemo latch_memo_;
  BplusTreeLogger logger_;
};