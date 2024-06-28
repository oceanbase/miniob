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

#include "common/rc.h"
#include "common/types.h"
#include "common/lang/mutex.h"
#include "common/lang/vector.h"
#include "common/lang/deque.h"
#include "common/lang/atomic.h"
#include "storage/clog/log_module.h"
#include "storage/clog/log_entry.h"

class LogFileWriter;

/**
 * @brief 日志数据缓冲区
 * @ingroup CLog
 * @details 缓存一部分日志在内存中而不是直接写入磁盘。
 * 这里的缓存没有考虑高性能操作，比如使用预分配内存、环形缓冲池等。
 * 在生产数据库中，通常会使用预分配内存形式的缓存来提高性能，并且可以减少内存碎片。
 */
class LogEntryBuffer
{
public:
  LogEntryBuffer()  = default;
  ~LogEntryBuffer() = default;

  RC init(LSN lsn, int32_t max_bytes = 0);

  /**
   * @brief 在缓冲区中追加一条日志
   */
  RC append(LSN &lsn, LogModule::Id module_id, vector<char> &&data);
  RC append(LSN &lsn, LogModule module, vector<char> &&data);

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
  mutex           mutex_;  /// 当前数据结构一定会在多线程中访问，所以强制使用有效的锁，而不是有条件生效的common::Mutex
  deque<LogEntry> entries_;  /// 日志缓冲区
  atomic<int64_t> bytes_;    /// 当前缓冲区中的日志数据大小

  atomic<LSN> current_lsn_{0};
  atomic<LSN> flushed_lsn_{0};

  int32_t max_bytes_ = 4 * 1024 * 1024;  /// 缓冲区最大字节数
};
