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
// Created by wangyunlai on 2022/02/01
//

#pragma once

#include <string>
#include "common/types.h"
#include "common/rc.h"

class DiskBufferPool;
class LogHandler;
struct Page;

class BufferPoolOperation
{
public:
  enum class Type : int32_t
  {
    ALLOCATE,
    DEALLOCATE
  };

public:
  BufferPoolOperation(Type type) : type_(type) {}
  BufferPoolOperation(int32_t type) : type_(static_cast<Type>(type)) {}
  ~BufferPoolOperation() = default;

  Type type() const { return type_; }
  int32_t type_id() const { return static_cast<int32_t>(type_); }

  std::string to_string() const
  {
    std::string ret = std::to_string(type_id()) + ":";
    switch (type_)
    {
    case Type::ALLOCATE:
      return ret + "ALLOCATE";
    case Type::DEALLOCATE:
      return ret + "DEALLOCATE";
    default:
      return ret + "UNKNOWN";
    }
  }

private:
  Type type_;
};

struct BufferPoolLogHeader
{
  int32_t  buffer_pool_id;  /// buffer pool id
  int32_t  operation_type;  /// operation type
  PageNum  page_num;        /// page number
};

class BufferPoolLogHandler final
{
public:
  BufferPoolLogHandler(DiskBufferPool &buffer_pool, LogHandler &log_handler);
  ~BufferPoolLogHandler() = default;

  RC allocate_page(PageNum page_num, LSN &lsn);
  RC deallocate_page(PageNum page_num, LSN &lsn);
  RC flush_page(Page &page);

private:
  RC append_log(BufferPoolOperation::Type type, PageNum page_num, LSN &lsn);

private:
  DiskBufferPool &buffer_pool_;
  LogHandler &log_handler_;
};