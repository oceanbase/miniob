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
// Created by wangyunlai on 2024/01/30
//

#include "common/thread/thread_util.h"
#include "storage/clog/disk_log_handler.h"
#include "storage/clog/log_file.h"
#include "storage/clog/log_replayer.h"
#include "common/lang/chrono.h"

using namespace common;

////////////////////////////////////////////////////////////////////////////////
// LogHandler

RC DiskLogHandler::init(const char *path)
{
  const int max_entry_number_per_file = 1000;
  return file_manager_.init(path, max_entry_number_per_file);
}

RC DiskLogHandler::start()
{
  if (thread_) {
    LOG_ERROR("log has been started");
    return RC::INTERNAL;
  }

  running_.store(true);
  thread_ = make_unique<thread>(&DiskLogHandler::thread_func, this);
  LOG_INFO("log handler started");
  return RC::SUCCESS;
}

RC DiskLogHandler::stop()
{
  if (!thread_) {
    LOG_ERROR("log has not been started");
    return RC::INTERNAL;
  }

  running_.store(false);

  LOG_INFO("log handler stopped");
  return RC::SUCCESS;
}

RC DiskLogHandler::await_termination()
{
  if (!thread_) {
    LOG_ERROR("log has not been started");
    return RC::INTERNAL;
  }

  if (running_.load()) {
    LOG_ERROR("log handler is running");
    return RC::INTERNAL;
  }

  thread_->join();
  thread_.reset();
  LOG_INFO("log handler joined");
  return RC::SUCCESS;
}

RC DiskLogHandler::replay(LogReplayer &replayer, LSN start_lsn)
{
  LSN max_lsn = 0;
  auto replay_callback = [&replayer, &max_lsn](LogEntry &entry) -> RC {
    if (entry.lsn() > max_lsn) {
      max_lsn = entry.lsn();
    }
    return replayer.replay(entry);
  };

  RC rc = iterate(replay_callback, start_lsn);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to iterate log entries. rc=%s", strrc(rc));
    return rc;
  }

  rc = entry_buffer_.init(max_lsn);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to init log entry buffer. rc=%s", strrc(rc));
    return rc;
  }

  LOG_INFO("replay clog files done. start lsn=%ld, max_lsn=%ld", start_lsn, max_lsn);
  return rc;
}

RC DiskLogHandler::iterate(function<RC(LogEntry&)> consumer, LSN start_lsn)
{
  vector<string> log_files;
  RC rc = file_manager_.list_files(log_files, start_lsn);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to list clog files. rc=%s", strrc(rc));
    return rc;
  }

  for (auto &file : log_files) {
    LogFileReader file_handle;
    rc = file_handle.open(file.c_str());
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to open clog file. rc=%s, file=%s", strrc(rc), file.c_str());
      return rc;
    }

    rc = file_handle.iterate(consumer, start_lsn);
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to iterate clog file. rc=%s, file=%s", strrc(rc), file.c_str());
      return rc;
    }

    rc = file_handle.close();
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to close clog file. rc=%s, file=%s", strrc(rc), file.c_str());
      return rc;
    }
  }

  LOG_INFO("iterate clog files done. rc=%s", strrc(rc));
  return RC::SUCCESS;
}

RC DiskLogHandler::_append(LSN &lsn, LogModule module, vector<char> &&data)
{
  ASSERT(running_.load(), "log handler is not running. lsn=%ld, module=%s, size=%d", 
        lsn, module.name(), data.size());

  RC rc = entry_buffer_.append(lsn, module, std::move(data));
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to append log entry to buffer. rc=%s", strrc(rc));
    return rc;
  }

  return RC::SUCCESS;
}

RC DiskLogHandler::wait_lsn(LSN lsn)
{
  // 直接强制等待。在生产系统中，我们可能会使用条件变量来等待。
  while (running_.load() && current_flushed_lsn() < lsn) {
    this_thread::sleep_for(chrono::milliseconds(100));
  }

  if (current_flushed_lsn() >= lsn) {
    return RC::SUCCESS;
  } else {
    return RC::INTERNAL;
  }
}

void DiskLogHandler::thread_func()
{
  /*
  这个线程一直不停的循环，检查日志缓冲区中是否有日志在内存中，如果在内存中就刷新到磁盘。
  这种做法很粗暴简单，与生产数据库的做法不同。生产数据库通常会在内存达到一定量，或者一定时间
  之后，会刷新一下磁盘。这样做的好处是减少磁盘的IO次数，提高性能。
  */
  thread_set_name("LogHandler");
  LOG_INFO("log handler thread started");

  LogFileWriter file_writer;
  
  RC rc = RC::SUCCESS;
  while (running_.load() || entry_buffer_.entry_number() > 0) {
    if (!file_writer.valid() || rc == RC::LOG_FILE_FULL) {
      if (rc == RC::LOG_FILE_FULL) {
        // 我们在这里判断日志文件是否写满了。
        rc = file_manager_.next_file(file_writer);
      } else {
        rc = file_manager_.last_file(file_writer);
      }
      if (OB_FAIL(rc)) {
        LOG_WARN("failed to open log file. rc=%s", strrc(rc));
        // 总是使用最简单的方法等待一段时间，期望错误会被修复。
        // 这在生产系统中是不被允许的。
        this_thread::sleep_for(chrono::milliseconds(100));
        continue;
      }
      LOG_INFO("open log file success. file=%s", file_writer.to_string().c_str());
    }

    int flush_count = 0;
    rc = entry_buffer_.flush(file_writer, flush_count);
    if (OB_FAIL(rc) && RC::LOG_FILE_FULL != rc) {
      LOG_WARN("failed to flush log entry buffer. rc=%s", strrc(rc));
    }

    if (flush_count == 0 && rc == RC::SUCCESS) {
      this_thread::sleep_for(chrono::milliseconds(100));
      continue;
    }
  }

  LOG_INFO("log handler thread stopped");
}
