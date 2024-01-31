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

#pragma once

#include <memory>
#include <deque>

#include "common/rc.h"
#include "common/types.h"
#include "common/lang/mutex.h"
#include "storage/clog/log_module.h"
#include "storage/clog/log_entry.h"

class LogFileWriter;

/**
 * @brief 日志数据缓冲区
 * @ingroup CLog
 * @details 缓存一部分日志在内存中而不是直接写入磁盘。
 * 这里的缓存没有考虑高性能操作，比如使用预分配内存、环形缓冲池等。
 */
class LogEntryBuffer
{
public:
  LogEntryBuffer() = default;
  ~LogEntryBuffer() = default;

  RC init(LSN lsn);

  /**
   * @brief 在缓冲区中追加一条日志
   */
  RC append(LSN &lsn, LogModule module, std::unique_ptr<char[]> data, int32_t size);

  /**
   * @brief 刷新缓冲区中的日志到磁盘
   * 
   * @param file_handle 使用它来写文件
   * @param count 刷了多少条日志
   */
  RC flush(LogFileWriter &file_writer, int &count);

  /**
   * @brief 当前缓冲区中有多少字节的日志
   */
  int64_t bytes() const;

  /**
   * @brief 当前缓冲区中有多少条日志
   */
  int32_t entry_number() const;

  LSN current_lsn() const { return current_lsn_.load(); }
  LSN flushed_lsn() const { return flushed_lsn_.load(); }

private:
  common::Mutex mutex_;
  std::deque<LogEntry> entries_;  /// 日志缓冲区
  std::atomic<int64_t> bytes_;    /// 当前缓冲区中的日志数据大小

  std::atomic<LSN> current_lsn_{0};
  std::atomic<LSN> flushed_lsn_{0};
};