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
// Created by Wangyunlai on 2023/4/25.
//

#pragma once

#include "common/rc.h"

class SQLStageEvent;

/**
 * @defgroup Executor
 * @brief 一些SQL语句不会生成对应的执行计划，直接使用Executor来执行，比如DDL语句
 */

/**
 * @brief 执行器
 * @ingroup Executor
 */
class CommandExecutor
{
public:
  CommandExecutor()          = default;
  virtual ~CommandExecutor() = default;

  RC execute(SQLStageEvent *sql_event);
};
