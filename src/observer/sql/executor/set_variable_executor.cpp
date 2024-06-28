/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "sql/executor/set_variable_executor.h"

RC SetVariableExecutor::execute(SQLStageEvent *sql_event)
{
    RC rc = RC::SUCCESS;

    Session         *session = sql_event->session_event()->session();
    SetVariableStmt *stmt    = (SetVariableStmt *)sql_event->stmt();

    const char  *var_name  = stmt->var_name();
    const Value &var_value = stmt->var_value();
    if (strcasecmp(var_name, "sql_debug") == 0) {
      bool bool_value = false;
      rc              = var_value_to_boolean(var_value, bool_value);
      if (rc == RC::SUCCESS) {
        session->set_sql_debug(bool_value);
        LOG_TRACE("set sql_debug to %d", bool_value);
      }
    } else if (strcasecmp(var_name, "execution_mode") == 0) {
      ExecutionMode  execution_mode = ExecutionMode::UNKNOWN_MODE;
      rc = get_execution_mode(var_value, execution_mode);
      if (rc == RC::SUCCESS && execution_mode != ExecutionMode::UNKNOWN_MODE) {
        session->set_execution_mode(execution_mode);
      } else {
        rc = RC::INVALID_ARGUMENT;
      }
    } else {
      rc = RC::VARIABLE_NOT_EXISTS;
    }

    return rc;
}

RC SetVariableExecutor::var_value_to_boolean(const Value &var_value, bool &bool_value) const
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

RC SetVariableExecutor::get_execution_mode(const Value &var_value, ExecutionMode &execution_mode) const
{
    RC rc = RC::SUCCESS;
  
    if (var_value.attr_type() == AttrType::CHARS) {
      if (strcasecmp(var_value.get_string().c_str(), "TUPLE_ITERATOR") == 0) {
        execution_mode = ExecutionMode::TUPLE_ITERATOR;
      } else if (strcasecmp(var_value.get_string().c_str(), "CHUNK_ITERATOR") == 0) {
        execution_mode = ExecutionMode::CHUNK_ITERATOR;
      } else {
        execution_mode = ExecutionMode::UNKNOWN_MODE;
        rc = RC::VARIABLE_NOT_VALID;
      }
    } else  {
      execution_mode = ExecutionMode::UNKNOWN_MODE;
      rc = RC::VARIABLE_NOT_VALID;
    }

    return rc;
}