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
#include "common/lang/functional.h"
#include "common/lang/memory.h"
#include "common/lang/span.h"
#include "common/lang/vector.h"
#include "storage/clog/log_module.h"

/**
 * @defgroup CLog commit log/redo log
 */

class LogReplayer;
class LogEntry;

/**
 * @brief 对外提供服务的CLog模块
 * @ingroup CLog
 * @details 该模块负责日志的写入、读取、回放等功能。
 * 会在后台开启一个线程，一直尝试刷新内存中的日志到磁盘。
 * 所有的CLog日志文件都存放在指定的目录下，每个日志文件按照日志条数来划分。
 */
class LogHandler
{
public:
  LogHandler()          = default;
  virtual ~LogHandler() = default;

  /**
   * @brief 初始化日志模块
   *
   * @param path 日志文件存放的目录
   */
  virtual RC init(const char *path) = 0;

  /**
   * @brief 启动日志模块
   */
  virtual RC start() = 0;

  /**
   * @brief 停止日志模块
   */
  virtual RC stop() = 0;

  /**
   * @brief 等待日志模块停止
   */
  virtual RC await_termination() = 0;

  /**
   * @brief 回放日志
   * @param replayer 日志回放器
   * @param start_lsn 从哪个LSN开始回放
   */
  virtual RC replay(LogReplayer &replayer, LSN start_lsn) = 0;

  /**
   * @brief 迭代日志
   * @param consumer 消费者
   * @param start_lsn 从哪个LSN开始迭代
   */
  virtual RC iterate(function<RC(LogEntry &)> consumer, LSN start_lsn) = 0;

  /**
   * @brief 写入一条日志
   * @param lsn 返回的LSN
   * @param module 日志模块
   * @param data 日志数据
   * @note 子类不应该重新实现这个函数
   */
  virtual RC append(LSN &lsn, LogModule::Id module, span<const char> data);
  virtual RC append(LSN &lsn, LogModule::Id module, vector<char> &&data);

  /**
   * @brief 等待某个LSN的日志被刷新到磁盘
   * @param lsn 日志的LSN
   */
  virtual RC wait_lsn(LSN lsn) = 0;

  virtual LSN current_lsn() const = 0;

  static RC create(const char *name, LogHandler *&handler);

private:
  /**
   * @brief 写入一条日志
   * @details 子类应该重现实现这个函数
   */
  virtual RC _append(LSN &lsn, LogModule module, vector<char> &&data) = 0;
};
