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

#pragma once

#include "common/rc.h"
#include "common/types.h"
#include "common/lang/map.h"
#include "common/lang/functional.h"
#include "common/lang/filesystem.h"
#include "common/lang/fstream.h"
#include "common/lang/string.h"

class LogEntry;

/**
 * @brief 负责处理一个日志文件，包括读取和写入
 * @ingroup CLog
 * @details 日志文件中的日志是按照LSN从小到大排列的
 */
class LogFileReader
{
public:
  LogFileReader()  = default;
  ~LogFileReader() = default;

  RC open(const char *filename);
  RC close();

  RC iterate(function<RC(LogEntry &)> callback, LSN start_lsn = 0);

private:
  /**
   * @brief 跳到第一条不小于start_lsn的日志
   *
   * @param start_lsn 期望开始的第一条日志的LSN
   */
  RC skip_to(LSN start_lsn);

private:
  int    fd_ = -1;
  string filename_;
};

/**
 * @brief 负责写入一个日志文件
 * @ingroup CLog
 */
class LogFileWriter
{
public:
  LogFileWriter() = default;
  ~LogFileWriter();

  /**
   * @brief 打开一个日志文件
   * @param filename 日志文件名
   * @param end_lsn 当前日志文件允许的最大LSN（包含）
   */
  RC open(const char *filename, int end_lsn);

  /// @brief 关闭当前文件
  RC close();

  /// @brief 写入一条日志
  RC write(LogEntry &entry);

  /**
   * @brief 当前文件是否已经打开
   */
  bool valid() const;

  /**
   * @brief 文件是否已经写满。当前是按照日志条数来判断的
   */
  bool full() const;

  string to_string() const;

  const char *filename() const { return filename_.c_str(); }

private:
  string filename_;       /// 日志文件名
  int    fd_       = -1;  /// 日志文件描述符
  int    last_lsn_ = 0;   /// 写入的最后一条日志LSN
  int    end_lsn_  = 0;   /// 当前日志文件中允许写入的最大的LSN，包括这条日志
};

/**
 * @brief 管理所有的日志文件
 * @ingroup CLog
 * @details 日志文件都在某个目录下，使用固定的前缀加上日志文件的第一个LSN作为文件名。
 * 每个日志文件没有最大字节数要求，但是以固定条数的日志为一个文件，这样方便查找。
 */
class LogFileManager
{
public:
  LogFileManager()  = default;
  ~LogFileManager() = default;

  /**
   * @brief 初始化
   *
   * @param directory 日志文件目录
   * @param max_entry_number_per_file 一个文件最多存储多少条日志
   */
  RC init(const char *directory, int max_entry_number_per_file);

  /**
   * @brief 列出所有的日志文件，第一个日志文件包含大于等于start_lsn最小的日志
   *
   * @param files 满足条件的所有日志文件名
   * @param start_lsn 想要查找的日志的最小LSN
   */
  RC list_files(vector<string> &files, LSN start_lsn);

  /**
   * @brief 获取最新的一个日志文件名
   * @details 如果当前有文件就获取最后一个日志文件，否则创建一个日志文件，也就是第一个日志文件
   */
  RC last_file(LogFileWriter &file_writer);

  /**
   * @brief 获取一个新的日志文件名
   * @details 获取下一个日志文件名。通常是上一个日志文件写满了，通过这个接口生成下一个日志文件
   */
  RC next_file(LogFileWriter &file_writer);

private:
  /**
   * @brief 从文件名称中获取LSN
   * @details 如果日志文件名不符合要求，就返回失败
   */
  static RC get_lsn_from_filename(const string &filename, LSN &lsn);

private:
  static constexpr const char *file_prefix_ = "clog_";
  static constexpr const char *file_suffix_ = ".log";

  filesystem::path directory_;                  /// 日志文件存放的目录
  int              max_entry_number_per_file_;  /// 一个文件最大允许存放多少条日志

  map<LSN, filesystem::path> log_files_;  /// 日志文件名和第一个LSN的映射
};
