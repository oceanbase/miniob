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

#include <string>
#include "common/rc.h"

class LogEntry;

/**
 * @brief 日志回放接口类
 * @ingroup CLog
 */
class LogReplayer
{
public:
  LogReplayer()          = default;
  virtual ~LogReplayer() = default;

  /**
   * @brief 回放一条日志
   *
   * @param entry 日志
   */
  virtual RC replay(const LogEntry &entry) = 0;

  /**
   * @brief 当所有日志回放完成时的回调函数
   */
  virtual RC on_done() { return RC::SUCCESS; }
};