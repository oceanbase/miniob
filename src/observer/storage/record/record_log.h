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
// Created by Wangyunlai on 2024/02/02.
//

#pragma once

#include <stdint.h>
#include <string>
#include "common/types.h"
#include "common/rc.h"
#include "storage/clog/log_replayer.h"

struct RID;
class LogHandler;
class Frame;
class BufferPoolManager;
class DiskBufferPool;

class RecordOperation
{
public:
  enum class Type : int32_t
  {
    INIT_PAGE,      /// 初始化空页面
    INSERT,
    DELETE,
    UPDATE
  };

public:
  explicit RecordOperation(Type type) : type_(type) {}
  explicit RecordOperation(int32_t type) : type_(static_cast<Type>(type)) {}
  ~RecordOperation() = default;

  Type type() const { return type_; }
  int32_t type_id() const { return static_cast<int32_t>(type_); }

  std::string to_string() const;

private:
  Type type_;
};

struct RecordLogHeader
{
  int32_t buffer_pool_id;
  int32_t operation_type;
  PageNum page_num;
  union {
    SlotNum slot_num;
    int32_t record_size;
  };

  char data[0];

  std::string to_string() const;

  static const int32_t SIZE;
};

class RecordLogHandler final
{
public:
  RecordLogHandler() = default;
  ~RecordLogHandler() = default;

  RC init(LogHandler &log_handler, int32_t buffer_pool_id, int32_t record_size);

  RC init_new_page(Frame *frame, PageNum page_num);
  RC insert_record(Frame *frame, const RID &rid, const char *record);
  RC delete_record(Frame *frame, const RID &rid);
  RC update_record(Frame *frame, const RID &rid, const char *record);

private:
  LogHandler *log_handler_ = nullptr;
  int32_t buffer_pool_id_ = -1;
  int32_t record_size_ = -1;
};

class RecordLogReplayer final : public LogReplayer
{
public:
  RecordLogReplayer(BufferPoolManager &bpm);
  virtual ~RecordLogReplayer() = default;

  virtual RC replay(const LogEntry &entry) override;

private:
  RC replay_init_page(DiskBufferPool &buffer_pool, const RecordLogHeader &log_header);
  RC replay_insert(DiskBufferPool &buffer_pool, const RecordLogHeader &log_header);
  RC replay_delete(DiskBufferPool &buffer_pool, const RecordLogHeader &log_header);
  RC replay_update(DiskBufferPool &buffer_pool, const RecordLogHeader &log_header);

private:
  BufferPoolManager &bpm_;
};
