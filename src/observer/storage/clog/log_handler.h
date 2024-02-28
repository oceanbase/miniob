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

#include <memory>
#include <functional>
#include <span>

#include "common/rc.h"
#include "common/types.h"
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
  LogHandler() = default;
  virtual ~LogHandler() = default;

  virtual RC replay(LogReplayer &replayer, LSN start_lsn) = 0;
  virtual RC iterate(std::function<RC(LogEntry&)> consumer, LSN start_lsn) = 0;

  virtual RC append(LSN &lsn, LogModule::Id module, std::span<const char> data);
  virtual RC append(LSN &lsn, LogModule::Id module, std::vector<char> &&data);

  virtual RC wait_lsn(LSN lsn) = 0;

private:
  virtual RC _append(LSN &lsn, LogModule module, std::vector<char> &&data) = 0;
};
