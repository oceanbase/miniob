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

  RC execute(SQLStageEvent *sql_event)
  {
    RC rc = RC::SUCCESS;

    Session         *session = sql_event->session_event()->session();
    SetVariableStmt *stmt    = (SetVariableStmt *)sql_event->stmt();

    const char  *var_name  = stmt->var_name();
    const Value &var_value = stmt->var_value();
    if (strcasecmp(var_name, "sql_debug") == 0) {
      bool bool_value = false;
      rc              = var_value_to_boolean(var_value, bool_value);
      if (rc != RC::SUCCESS) {
        return rc;
      }

      session->set_sql_debug(bool_value);
      LOG_TRACE("set sql_debug to %d", bool_value);
    } else {
      rc = RC::VARIABLE_NOT_EXISTS;
    }

    return RC::SUCCESS;
  }

private:
  RC var_value_to_boolean(const Value &var_value, bool &bool_value) const
  {
    RC rc = RC::SUCCESS;

    if (var_value.attr_type() == AttrType::BOOLEANS) {
      bool_value = var_value.get_boolean();
    } else if (var_value.attr_type() == AttrType::INTS) {
      bool_value = var_value.get_int() != 0;
    } else if (var_value.attr_type() == AttrType::FLOATS) {
      bool_value = var_value.get_float() != 0.0;
    } else if (var_value.attr_type() == AttrType::CHARS) {

      std::string true_strings[] = {"true", "on", "yes", "t", "1"};

      std::string false_strings[] = {"false", "off", "no", "f", "0"};

      for (size_t i = 0; i < sizeof(true_strings) / sizeof(true_strings[0]); i++) {
        if (strcasecmp(var_value.get_string().c_str(), true_strings[i].c_str()) == 0) {
          bool_value = true;
          return rc;
        }
      }

      for (size_t i = 0; i < sizeof(false_strings) / sizeof(false_strings[0]); i++) {
        if (strcasecmp(var_value.get_string().c_str(), false_strings[i].c_str()) == 0) {
          bool_value = false;
          return rc;
        }
      }
      rc = RC::VARIABLE_NOT_VALID;
    }

    return rc;
  }
};