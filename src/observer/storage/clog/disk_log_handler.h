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
// Created by wangyunlai on 2024/02/01
//

#pragma once

#include <thread>
#include <memory>
#include <deque>
#include <vector>

#include "common/types.h"
#include "common/rc.h"
#include "storage/clog/log_module.h"
#include "storage/clog/log_file.h"
#include "storage/clog/log_buffer.h"
#include "storage/clog/log_handler.h"

class LogReplayer;

/**
 * @brief 对外提供服务的CLog模块
 * @ingroup CLog
 * @details 该模块负责日志的写入、读取、回放等功能。
 * 会在后台开启一个线程，一直尝试刷新内存中的日志到磁盘。
 * 所有的CLog日志文件都存放在指定的目录下，每个日志文件按照日志条数来划分。
 */
class DiskLogHandler : public LogHandler
{
public:
  DiskLogHandler() = default;
  virtual ~DiskLogHandler() = default;

  /**
   * @brief 初始化日志模块
   * 
   * @param path 日志文件存放的目录
   */
  RC init(const char *path);
  RC start();
  RC stop();
  RC wait();

  RC replay(LogReplayer &replayer, LSN start_lsn) override;
  RC iterate(std::function<RC(LogEntry&)> consumer, LSN start_lsn) override;

  RC wait_lsn(LSN lsn) override;

  LSN current_lsn() const { return entry_buffer_.current_lsn(); }
  LSN current_flushed_lsn() const { return entry_buffer_.flushed_lsn(); }

private:
  RC _append(LSN &lsn, LogModule module, std::unique_ptr<char[]> data, int32_t size) override;
private:
  void thread_func();

private:
  std::unique_ptr<std::thread> thread_;
  std::atomic_bool running_{false};

  LogFileManager file_manager_;
  LogEntryBuffer entry_buffer_;

  std::string path_;
};
