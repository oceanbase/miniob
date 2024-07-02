/* Copyright (c) 2021-2022 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by wangyunlai on 2024/01/31
//

#include "storage/clog/log_buffer.h"
#include "storage/clog/log_file.h"
#include "common/lang/chrono.h"

using namespace common;

RC LogEntryBuffer::init(LSN lsn, int32_t max_bytes /*= 0*/)
{
  current_lsn_.store(lsn);
  flushed_lsn_.store(lsn);

  if (max_bytes > 0) {
    max_bytes_ = max_bytes;
  }
  return RC::SUCCESS;
}

RC LogEntryBuffer::append(LSN &lsn, LogModule::Id module_id, vector<char> &&data)
{
  return append(lsn, LogModule(module_id), std::move(data));
}

RC LogEntryBuffer::append(LSN &lsn, LogModule module, vector<char> &&data)
{
  /// 控制当前buffer使用的内存
  /// 简单粗暴，强制原地等待
  /// 但是如果当前想要新插入的日志比较大，不会做控制。所以理论上容纳的最大buffer内存是2*max_bytes_
  while (bytes_.load() >= max_bytes_) {
    this_thread::sleep_for(chrono::milliseconds(10));
  }

  LogEntry entry;
  RC rc = entry.init(lsn, module, std::move(data));
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to init log entry. rc=%s", strrc(rc));
    return rc;
  }

  lock_guard guard(mutex_);
  lsn = ++current_lsn_;
  entry.set_lsn(lsn);

  entries_.push_back(std::move(entry));
  bytes_ += entry.total_size();
  return RC::SUCCESS;
}

RC LogEntryBuffer::flush(LogFileWriter &writer, int &count)
{
  count = 0;

  while (entry_number() > 0) {
    LogEntry entry;
    {
      lock_guard guard(mutex_);
      if (entries_.empty()) {
        break;
      }

      LogEntry &front_entry = entries_.front();
      ASSERT(front_entry.lsn() > 0 && front_entry.payload_size() > 0, "invalid log entry");
      entry = std::move(entries_.front());
      ASSERT(entry.payload_size() > 0 && entry.lsn() > 0, "invalid log entry");
      entries_.pop_front();
      bytes_ -= entry.total_size();
    }
    
    RC rc = writer.write(entry);
    if (OB_FAIL(rc)) {
      lock_guard guard(mutex_);
      entries_.emplace_front(std::move(entry));
      LogEntry &front_entry = entries_.front();
      ASSERT(front_entry.lsn() > 0 && front_entry.payload_size() > 0, "invalid log entry");
      return rc;
    } else {
      ++count;
      flushed_lsn_ = entry.lsn();
    }
  }
  
  return RC::SUCCESS;
}

int64_t LogEntryBuffer::bytes() const
{
  return bytes_.load();
}

int32_t LogEntryBuffer::entry_number() const
{
  return entries_.size();
}
