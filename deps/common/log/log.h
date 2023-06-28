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

#pragma once

#include <sys/time.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>

#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <functional>

#include "common/defs.h"

namespace common {

const unsigned int ONE_KILO = 1024;
const unsigned int FILENAME_LENGTH_MAX = 256;  // the max filename length

const int LOG_STATUS_OK = 0;
const int LOG_STATUS_ERR = 1;
const int LOG_MAX_LINE = 100000;

typedef enum {
  LOG_LEVEL_PANIC = 0,
  LOG_LEVEL_ERR = 1,
  LOG_LEVEL_WARN = 2,
  LOG_LEVEL_INFO = 3,
  LOG_LEVEL_DEBUG = 4,
  LOG_LEVEL_TRACE = 5,
  LOG_LEVEL_LAST
} LOG_LEVEL;

typedef enum { LOG_ROTATE_BYDAY = 0, LOG_ROTATE_BYSIZE, LOG_ROTATE_LAST } LOG_ROTATE;

class Log 
{
public:
  Log(const std::string &log_name, const LOG_LEVEL log_level = LOG_LEVEL_INFO,
      const LOG_LEVEL console_level = LOG_LEVEL_WARN);
  ~Log(void);

  static int init(const std::string &log_file);

  /**
   * These functions won't output header information such as __FUNCTION__,
   * The user should control these information
   * If the header information should be outputed
   * please use LOG_PANIC, LOG_ERROR ...
   */
  template <class T>
  Log &operator<<(T message);

  template <class T>
  int panic(T message);

  template <class T>
  int error(T message);

  template <class T>
  int warnning(T message);

  template <class T>
  int info(T message);

  template <class T>
  int debug(T message);

  template <class T>
  int trace(T message);

  int output(const LOG_LEVEL level, const char *module, const char *prefix, const char *f, ...);

  int set_console_level(const LOG_LEVEL console_level);
  LOG_LEVEL get_console_level();

  int set_log_level(const LOG_LEVEL log_level);
  LOG_LEVEL get_log_level();

  int set_rotate_type(LOG_ROTATE rotate_type);
  LOG_ROTATE get_rotate_type();

  const char *prefix_msg(const LOG_LEVEL level);

  /**
   * Set Default Module list
   * if one module is default module,
   * it will output whatever output level is lower than log_level_ or not
   */
  void set_default_module(const std::string &modules);
  bool check_output(const LOG_LEVEL log_level, const char *module);

  int rotate(const int year = 0, const int month = 0, const int day = 0);

  /**
   * @brief 设置一个在日志中打印当前上下文信息的回调函数
   * @details 比如设置一个获取当前session标识的函数，那么每次在打印日志时都会输出session信息。
   *          这个回调函数返回了一个intptr_t类型的数据，可能返回字符串更好，但是现在够用了。
   */
  void set_context_getter(std::function<intptr_t()> context_getter);
  intptr_t context_id();
  
private:
  void check_param_valid();

  int rotate_by_size();
  int rename_old_logs();
  int rotate_by_day(const int year, const int month, const int day);

  template <class T>
  int out(const LOG_LEVEL console_level, const LOG_LEVEL log_level, T &message);

private:
  pthread_mutex_t lock_;
  std::ofstream ofs_;
  std::string log_name_;
  LOG_LEVEL log_level_;
  LOG_LEVEL console_level_;

  typedef struct _LogDate {
    int year_;
    int mon_;
    int day_;
  } LogDate;
  LogDate log_date_;
  int log_line_;
  int log_max_line_;
  LOG_ROTATE rotate_type_;

  typedef std::map<LOG_LEVEL, std::string> LogPrefixMap;
  LogPrefixMap prefix_map_;

  typedef std::set<std::string> DefaultSet;
  DefaultSet default_set_;

  std::function<intptr_t()> context_getter_;
};

class LoggerFactory {
public:
  LoggerFactory();
  virtual ~LoggerFactory();

  static int init(const std::string &log_file, Log **logger, LOG_LEVEL log_level = LOG_LEVEL_INFO,
      LOG_LEVEL console_level = LOG_LEVEL_WARN, LOG_ROTATE rotate_type = LOG_ROTATE_BYDAY);

  static int init_default(const std::string &log_file, LOG_LEVEL log_level = LOG_LEVEL_INFO,
      LOG_LEVEL console_level = LOG_LEVEL_WARN, LOG_ROTATE rotate_type = LOG_ROTATE_BYDAY);
};

extern Log *g_log;

#ifndef __FILE_NAME__
#define __FILE_NAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#define LOG_HEAD_SIZE 128

#define LOG_HEAD(prefix, level)                                            \
  if (common::g_log) {                                                     \
    struct timeval tv;                                                     \
    gettimeofday(&tv, NULL);                                               \
    struct tm *p = localtime(&tv.tv_sec);                                  \
    char sz_head[LOG_HEAD_SIZE] = {0};                                     \
    if (p) {                                                               \
      int usec = (int)tv.tv_usec;                                          \
      snprintf(sz_head, LOG_HEAD_SIZE,                                     \
          "%04d-%02d-%02d %02d:%02d:%02u.%06d pid:%u tid:%llx ctx:%lx",    \
          p->tm_year + 1900,                                               \
          p->tm_mon + 1,                                                   \
          p->tm_mday,                                                      \
          p->tm_hour,                                                      \
          p->tm_min,                                                       \
          p->tm_sec,                                                       \
          usec,                                                            \
          (int32_t)getpid(),                                               \
          gettid(),                                                        \
          common::g_log->context_id());                                    \
      common::g_log->rotate(p->tm_year + 1900, p->tm_mon + 1, p->tm_mday); \
    }                                                                      \
    snprintf(prefix,                                                       \
        sizeof(prefix),                                                    \
        "[%s %s %s@%s:%u] >> ",                                            \
        sz_head,                                                           \
        (common::g_log)->prefix_msg(level),                                \
        __FUNCTION__,                                                      \
        __FILE_NAME__,                                                     \
        (int32_t)__LINE__                                                  \
        );                                                                 \
  }

#define LOG_OUTPUT(level, fmt, ...)                                    \
  do {                                                                 \
    using namespace common;                                            \
    if (g_log && g_log->check_output(level, __FILE_NAME__)) {          \
      char prefix[ONE_KILO] = {0};                                     \
      LOG_HEAD(prefix, level);                                         \
      g_log->output(level, __FILE_NAME__, prefix, fmt, ##__VA_ARGS__); \
    }                                                                  \
  } while (0)

#define LOG_DEFAULT(fmt, ...) LOG_OUTPUT(common::g_log->get_log_level(), fmt, ##__VA_ARGS__)
#define LOG_PANIC(fmt, ...) LOG_OUTPUT(common::LOG_LEVEL_PANIC, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) LOG_OUTPUT(common::LOG_LEVEL_ERR, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) LOG_OUTPUT(common::LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) LOG_OUTPUT(common::LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) LOG_OUTPUT(common::LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define LOG_TRACE(fmt, ...) LOG_OUTPUT(common::LOG_LEVEL_TRACE, fmt, ##__VA_ARGS__)

template <class T>
Log &Log::operator<<(T msg)
{
  // at this time, the input level is the default log level
  out(console_level_, log_level_, msg);
  return *this;
}

template <class T>
int Log::panic(T message)
{
  return out(LOG_LEVEL_PANIC, LOG_LEVEL_PANIC, message);
}

template <class T>
int Log::error(T message)
{
  return out(LOG_LEVEL_ERR, LOG_LEVEL_ERR, message);
}

template <class T>
int Log::warnning(T message)
{
  return out(LOG_LEVEL_WARN, LOG_LEVEL_WARN, message);
}

template <class T>
int Log::info(T message)
{
  return out(LOG_LEVEL_INFO, LOG_LEVEL_INFO, message);
}

template <class T>
int Log::debug(T message)
{
  return out(LOG_LEVEL_DEBUG, LOG_LEVEL_DEBUG, message);
}

template <class T>
int Log::trace(T message)
{
  return out(LOG_LEVEL_TRACE, LOG_LEVEL_TRACE, message);
}

template <class T>
int Log::out(const LOG_LEVEL console_level, const LOG_LEVEL log_level, T &msg)
{
  bool locked = false;
  if (console_level < LOG_LEVEL_PANIC || console_level > console_level_ || log_level < LOG_LEVEL_PANIC ||
      log_level > log_level_) {
    return LOG_STATUS_OK;
  }
  try {
    char prefix[ONE_KILO] = {0};
    LOG_HEAD(prefix, log_level);
    if (LOG_LEVEL_PANIC <= console_level && console_level <= console_level_) {
      std::cout << prefix_map_[console_level] << msg;
    }

    if (LOG_LEVEL_PANIC <= log_level && log_level <= log_level_) {
      pthread_mutex_lock(&lock_);
      locked = true;
      ofs_ << prefix;
      ofs_ << msg;
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

#ifndef ASSERT
#ifdef DEBUG
#define ASSERT(expression, description, ...)   \
  do {                                         \
    if (!(expression)) {                       \
      LOG_PANIC(description, ##__VA_ARGS__);   \
      assert(expression);                      \
    }                                          \
  } while (0)

#else // DEBUG
#define ASSERT(expression, description, ...)   \
  do {                                         \
     (void)(expression);                       \
  } while (0)
#endif // DEBUG

#endif  // ASSERT

#define SYS_OUTPUT_FILE_POS ", File:" << __FILE__ << ", line:" << __LINE__ << ",function:" << __FUNCTION__
#define SYS_OUTPUT_ERROR ",error:" << errno << ":" << strerror(errno)

/**
 * 获取当前函数调用栈
 */
const char *lbt();

}  // namespace common
