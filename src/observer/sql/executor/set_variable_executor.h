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
// Created by Wangyunlai on 2023/6/14.
//

#pragma once

#include "common/rc.h"
#include "event/session_event.h"
#include "event/sql_event.h"
#include "session/session.h"
#include "sql/stmt/set_variable_stmt.h"

/**
 * @brief SetVariable语句执行器
 * @ingroup Executor
 */
class SetVariableExecutor
{
public:
  SetVariableExecutor()          = default;
  virtual ~SetVariableExecutor() = default;

  RC execute(SQLStageEvent *sql_event);

private:
  RC var_value_to_boolean(const Value &var_value, bool &bool_value) const;

  RC get_execution_mode(const Value &var_value, ExecutionMode &execution_mode) const;
};