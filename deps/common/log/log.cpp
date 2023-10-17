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
// Created by Longda on 2010
//

#include <assert.h>
#include <exception>
#include <stdarg.h>
#include <stdio.h>
#include <execinfo.h>

#include "common/lang/string.h"
#include "common/log/log.h"
namespace common {

Log *g_log = nullptr;

Log::Log(const std::string &log_file_name, const LOG_LEVEL log_level, const LOG_LEVEL console_level)
    : log_name_(log_file_name), log_level_(log_level), console_level_(console_level)
{
  prefix_map_[LOG_LEVEL_PANIC] = "PANIC:";
  prefix_map_[LOG_LEVEL_ERR] = "ERROR:";
  prefix_map_[LOG_LEVEL_WARN] = "WARN:";
  prefix_map_[LOG_LEVEL_INFO] = "INFO:";
  prefix_map_[LOG_LEVEL_DEBUG] = "DEBUG:";
  prefix_map_[LOG_LEVEL_TRACE] = "TRACE:";

  pthread_mutex_init(&lock_, nullptr);

  log_date_.year_ = -1;
  log_date_.mon_ = -1;
  log_date_.day_ = -1;
  log_max_line_ = LOG_MAX_LINE;
  log_line_ = -1;
  rotate_type_ = LOG_ROTATE_BYDAY;

  check_param_valid();

  context_getter_ = []() { return 0; };
}

Log::~Log(void)
{
  pthread_mutex_lock(&lock_);
  if (ofs_.is_open()) {
    ofs_.close();
  }
  pthread_mutex_unlock(&lock_);

  pthread_mutex_destroy(&lock_);
}

void Log::check_param_valid()
{
  assert(!log_name_.empty());
  assert(LOG_LEVEL_PANIC <= log_level_ && log_level_ < LOG_LEVEL_LAST);
  assert(LOG_LEVEL_PANIC <= console_level_ && console_level_ < LOG_LEVEL_LAST);

  return;
}

bool Log::check_output(const LOG_LEVEL level, const char *module)
{
  if (LOG_LEVEL_LAST > level && level <= console_level_) {
    return true;
  }
  if (LOG_LEVEL_LAST > level && level <= log_level_) {
    return true;
  }
  // in order to improve speed
  if (default_set_.empty() == false && default_set_.find(module) != default_set_.end()) {
    return true;
  }
  return false;
}

int Log::output(const LOG_LEVEL level, const char *module, const char *prefix, const char *f, ...)
{
  bool locked = false;
  try {
    va_list args;
    char msg[ONE_KILO];

    va_start(args, f);
    vsnprintf(msg, sizeof(msg), f, args);
    va_end(args);

    if (LOG_LEVEL_PANIC <= level && level <= console_level_) {
      std::cout << msg << std::endl;
    } else if (default_set_.find(module) != default_set_.end()) {
      std::cout << msg << std::endl;
    }

    if (LOG_LEVEL_PANIC <= level && level <= log_level_) {
      pthread_mutex_lock(&lock_);
      locked = true;
      ofs_ << prefix;
      ofs_ << msg;
      ofs_ << "\n";
      ofs_.flush();
      log_line_++;
      pthread_mutex_unlock(&lock_);
      locked = false;
    } else if (default_set_.find(module) != default_set_.end()) {
      pthread_mutex_lock(&lock_);
      locked = true;
      ofs_ << prefix;
      ofs_ << msg;
      ofs_ << "\n";
      ofs_.flush();
      log_line_++;
      pthread_mutex_unlock(&lock_);
      locked = false;
    }

  } catch (std::exception &e) {
    if (locked) {
      pthread_mutex_unlock(&lock_);
    }
    std::cerr << e.what() << std::endl;
    return LOG_STATUS_ERR;
  }

  return LOG_STATUS_OK;
}

int Log::set_console_level(LOG_LEVEL console_level)
{
  if (LOG_LEVEL_PANIC <= console_level && console_level < LOG_LEVEL_LAST) {
    console_level_ = console_level;
    return LOG_STATUS_OK;
  }

  return LOG_STATUS_ERR;
}

LOG_LEVEL Log::get_console_level()
{
  return console_level_;
}

int Log::set_log_level(LOG_LEVEL log_level)
{
  if (LOG_LEVEL_PANIC <= log_level && log_level < LOG_LEVEL_LAST) {
    log_level_ = log_level;
    return LOG_STATUS_OK;
  }

  return LOG_STATUS_ERR;
}

LOG_LEVEL Log::get_log_level()
{
  return log_level_;
}

const char *Log::prefix_msg(LOG_LEVEL level)
{
  if (LOG_LEVEL_PANIC <= level && level < LOG_LEVEL_LAST) {
    return prefix_map_[level].c_str();
  }
  static const char *empty_prefix = "";
  return empty_prefix;
}

void Log::set_default_module(const std::string &modules)
{
  split_string(modules, ",", default_set_);
}

int Log::set_rotate_type(LOG_ROTATE rotate_type)
{
  if (LOG_ROTATE_BYDAY <= rotate_type && rotate_type < LOG_ROTATE_LAST) {
    rotate_type_ = rotate_type;
  }
  return LOG_STATUS_OK;
}

LOG_ROTATE Log::get_rotate_type()
{
  return rotate_type_;
}

int Log::rotate_by_day(const int year, const int month, const int day)
{
  if (log_date_.year_ == year && log_date_.mon_ == month && log_date_.day_ == day) {
    // Don't need rotate
    return 0;
  }

  char date[16] = {0};
  snprintf(date, sizeof(date), "%04d%02d%02d", year, month, day);
  std::string log_file_name = log_name_ + "." + date;

  if (ofs_.is_open()) {
    ofs_.close();
  }
  ofs_.open(log_file_name.c_str(), std::ios_base::out | std::ios_base::app);
  if (ofs_.good()) {
    log_date_.year_ = year;
    log_date_.mon_ = month;
    log_date_.day_ = day;

    log_line_ = 0;
  }

  return 0;
}

#define MAX_LOG_NUM 999

int Log::rename_old_logs()
{
  int log_index = 1;
  int max_log_index = 0;

  while (log_index < MAX_LOG_NUM) {
    std::string log_name = log_name_ + "." + size_to_pad_str(log_index, 3);
    int result = access(log_name.c_str(), R_OK);
    if (result) {
      break;
    }

    max_log_index = log_index;
    log_index++;
  }

  if (log_index == MAX_LOG_NUM) {
    std::string log_name_rm = log_name_ + "." + size_to_pad_str(log_index, 3);
    remove(log_name_rm.c_str());
  }

  log_index = max_log_index;
  while (log_index > 0) {
    std::string log_name_old = log_name_ + "." + size_to_pad_str(log_index, 3);

    std::string log_name_new = log_name_ + "." + size_to_pad_str(log_index + 1, 3);

    int result = rename(log_name_old.c_str(), log_name_new.c_str());
    if (result) {
      return LOG_STATUS_ERR;
    }
    log_index--;
  }

  return LOG_STATUS_OK;
}

int Log::rotate_by_size()
{
  if (log_line_ < 0) {
    // The first time open log file
    ofs_.open(log_name_.c_str(), std::ios_base::out | std::ios_base::app);
    log_line_ = 0;
    return LOG_STATUS_OK;
  } else if (0 <= log_line_ && log_line_ < log_max_line_) {
    // Don't need rotate
    return LOG_STATUS_OK;
  } else {

    int result = rename_old_logs();
    if (result) {
      // rename old logs failed
      return LOG_STATUS_OK;
    }

    if (ofs_.is_open()) {
      ofs_.close();
    }

    char log_index_str[4] = {0};
    snprintf(log_index_str, sizeof(log_index_str), "%03d", 1);
    std::string log_name_new = log_name_ + "." + log_index_str;
    result = rename(log_name_.c_str(), log_name_new.c_str());
    if (result) {
      std::cerr << "Failed to rename " << log_name_ << " to " << log_name_new << std::endl;
    }

    ofs_.open(log_name_.c_str(), std::ios_base::out | std::ios_base::app);
    if (ofs_.good()) {
      log_line_ = 0;
    } else {
      // Error
      log_line_ = log_max_line_;
    }

    return LOG_STATUS_OK;
  }

  return LOG_STATUS_OK;
}

int Log::rotate(const int year, const int month, const int day)
{
  int result = 0;
  pthread_mutex_lock(&lock_);
  if (rotate_type_ == LOG_ROTATE_BYDAY) {
    result = rotate_by_day(year, month, day);
  } else {
    result = rotate_by_size();
  }
  pthread_mutex_unlock(&lock_);

  return result;
}

void Log::set_context_getter(std::function<intptr_t()> context_getter)
{
  if (context_getter) {
    context_getter_ = context_getter;
  } else if (!context_getter_) {
    context_getter_ = []() { return 0; };
  }
}

intptr_t Log::context_id()
{
  return context_getter_();
}

LoggerFactory::LoggerFactory()
{
  // Auto-generated constructor stub
}

LoggerFactory::~LoggerFactory()
{
  // Auto-generated destructor stub
}

int LoggerFactory::init(
    const std::string &log_file, Log **logger, LOG_LEVEL log_level, LOG_LEVEL console_level, LOG_ROTATE rotate_type)
{
  Log *log = new (std::nothrow) Log(log_file, log_level, console_level);
  if (log == nullptr) {
    std::cout << "Error: fail to construct a log object!" << std::endl;
    return -1;
  }
  log->set_rotate_type(rotate_type);

  *logger = log;

  return 0;
}

int LoggerFactory::init_default(
    const std::string &log_file, LOG_LEVEL log_level, LOG_LEVEL console_level, LOG_ROTATE rotate_type)
{
  if (g_log != nullptr) {
    LOG_INFO("Default logger has been initialized");
    return 0;
  }

  return init(log_file, &g_log, log_level, console_level, rotate_type);
}

const char *lbt()
{
  constexpr int buffer_size = 100;
  void *buffer[buffer_size];
  
  constexpr int bt_buffer_size = 4096;
  thread_local char backtrace_buffer[bt_buffer_size];

  int size = backtrace(buffer, buffer_size);
  int offset = 0;
  for (int i = 0; i < size && offset < bt_buffer_size - 1; i++) {
    const char *format = (0 == i) ? "0x%lx" : " 0x%lx";
    offset += snprintf(backtrace_buffer + offset, sizeof(backtrace_buffer) - offset, format,
                       reinterpret_cast<intptr_t>(buffer[i]));
  }
  return backtrace_buffer;
}

}  // namespace common
