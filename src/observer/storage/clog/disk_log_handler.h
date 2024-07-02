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

#include "common/types.h"
#include "common/rc.h"
#include "common/lang/vector.h"
#include "common/lang/deque.h"
#include "common/lang/memory.h"
#include "common/lang/thread.h"
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
 * 调用的顺序应该是：
 * @code {.cpp}
 * DiskLogHandler handler;
 * handler.init("/path/to/log");
 * // replay 在一次启动只运行一次，它会计算当前日志的一些最新状态，必须在start之前执行
 * handler.replay(replayer, start_lsn);
 * handler.start();
 * handler.stop();
 * handler.wait();
 * @endcode
 *
 */
class DiskLogHandler : public LogHandler
{
public:
  DiskLogHandler()          = default;
  virtual ~DiskLogHandler() = default;

  /**
   * @brief 初始化日志模块
   *
   * @param path 日志文件存放的目录
   */
  RC init(const char *path) override;

  /**
   * @brief 启动线程刷新日志到磁盘
   */
  RC start() override;
  /**
   * @brief 设置停止标识，并不会真正的停止
   */
  RC stop() override;

  /**
   * @brief 等待线程结束
   * @details 会刷新完所有日志到磁盘
   */
  RC await_termination() override;

  /**
   * @brief 回放日志
   * @details 日志回放后，会记录当前日志的最新状态，包括当前最大的LSN。
   * 所以这个接口应该在启动之前调用一次。
   * @param replayer 回放日志接口
   * @param start_lsn 从哪个位置开始回放
   */
  RC replay(LogReplayer &replayer, LSN start_lsn) override;

  /**
   * @brief 迭代日志
   * @details 从start_lsn开始，迭代所有的日志。这仅仅是一个辅助函数。
   * @param consumer 消费者
   * @param start_lsn 从哪个位置开始迭代
   */
  RC iterate(function<RC(LogEntry &)> consumer, LSN start_lsn) override;

  /**
   * @brief 等待指定的日志刷盘
   *
   * @param lsn 想要等待的日志
   */
  RC wait_lsn(LSN lsn) override;

  /// @brief 当前的LSN
  LSN current_lsn() const override { return entry_buffer_.current_lsn(); }
  /// @brief 当前刷新到哪个日志
  LSN current_flushed_lsn() const { return entry_buffer_.flushed_lsn(); }

private:
  /**
   * @brief 在缓存中增加一条日志
   *
   * @param[out] lsn    返回的LSN
   * @param[in] module  日志模块
   * @param[in] data    日志数据。具体的数据由各个模块自己定义
   */
  RC _append(LSN &lsn, LogModule module, vector<char> &&data) override;

private:
  /**
   * @brief 刷新日志的线程函数
   */
  void thread_func();

private:
  unique_ptr<thread> thread_;          /// 刷新日志的线程
  atomic_bool        running_{false};  /// 是否还要继续运行

  LogFileManager file_manager_;  /// 管理所有的日志文件
  LogEntryBuffer entry_buffer_;  /// 缓存日志

  string path_;  /// 日志文件存放的目录
};
