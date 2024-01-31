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
#include "storage/clog/log_handler.h"
#include "storage/clog/log_file.h"
#include "storage/clog/log_replayer.h"

using namespace std;
using namespace common;

////////////////////////////////////////////////////////////////////////////////

RC LogEntryBuffer::init(LSN lsn)
{
  current_lsn_.store(lsn);
  flushed_lsn_.store(lsn);
  return RC::SUCCESS;
}

RC LogEntryBuffer::append(LSN &lsn, LogModule module, std::unique_ptr<char[]> data, int32_t size)
{
  lock_guard guard(mutex_);
  lsn = ++current_lsn_;

  LogEntry entry;
  RC rc = entry.init(lsn, module, std::move(data), size);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to init log entry. rc=%s", strrc(rc));
    return rc;
  }

  entries_.emplace_back(std::move(entry));
  bytes_ += entry.size();
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

      entry = std::move(entries_.front());
      entries_.pop_front();
      bytes_ -= entry.size();
    }
    
    RC rc = writer.write(entry);
    if (OB_FAIL(rc)) {
      lock_guard guard(mutex_);
      entries_.emplace_front(std::move(entry));
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

////////////////////////////////////////////////////////////////////////////////
// LogHandler

RC LogHandler::init(const char *path)
{
  const int max_entry_number_per_file = 1000;
  return file_manager_.init(path, max_entry_number_per_file);
}

RC LogHandler::start()
{
  if (thread_) {
    LOG_ERROR("log has been started");
    return RC::INTERNAL;
  }

  running_.store(true);
  thread_ = make_unique<thread>(&LogHandler::thread_func, this);
  LOG_INFO("log handler started");
  return RC::SUCCESS;
}

RC LogHandler::stop()
{
  if (!thread_) {
    LOG_ERROR("log has not been started");
    return RC::INTERNAL;
  }

  running_.store(false);

  LOG_INFO("log handler stopped");
  return RC::SUCCESS;
}

RC LogHandler::wait()
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

RC LogHandler::replay(LogReplayer &replayer, LSN start_lsn)
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
  return RC::SUCCESS;
}

RC LogHandler::iterate(std::function<RC(LogEntry&)> consumer, LSN start_lsn)
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

RC LogHandler::append(LSN &lsn, LogModule module, unique_ptr<char[]> data, int32_t size)
{
  RC rc = entry_buffer_.append(lsn, module, std::move(data), size);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to append log entry to buffer. rc=%s", strrc(rc));
    return rc;
  }

  return RC::SUCCESS;
}

RC LogHandler::wait_lsn(LSN lsn)
{
  while (running_.load() && current_flushed_lsn() < lsn) {
    this_thread::sleep_for(chrono::milliseconds(1000));
  }

  return RC::SUCCESS;
}

void LogHandler::thread_func()
{
  thread_set_name("LogHandler");
  LOG_INFO("log handler thread started");

  LogFileWriter file_writer;
  
  while (running_.load() && entry_buffer_.entry_number() > 0) {
    if (!file_writer.valid() || file_writer.full()) {
      RC rc = RC::SUCCESS;
      if (file_writer.full()) {
        rc = file_manager_.next_file(file_writer);
      } else {
        rc = file_manager_.last_file(file_writer);
      }
      if (OB_FAIL(rc)) {
        LOG_WARN("failed to open log file. rc=%s", strrc(rc));
        this_thread::sleep_for(chrono::milliseconds(1000));
        continue;
      }
      LOG_INFO("open log file success. file=%s", file_writer.to_string().c_str());
    }

    int flush_count = 0;
    RC rc = entry_buffer_.flush(file_writer, flush_count);
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to flush log entry buffer. rc=%s", strrc(rc));
    }

    if (flush_count == 0) {
      this_thread::sleep_for(chrono::milliseconds(1000));
      continue;
    }
  }

  LOG_INFO("log handler thread stopped");
}