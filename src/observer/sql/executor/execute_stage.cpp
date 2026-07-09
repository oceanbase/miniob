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
// Created by Longda on 2021/4/13.
//

#include "sql/executor/execute_stage.h"

#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <strings.h> // 引入 strcasecmp

#include "common/log/log.h"
#include "common/lang/string.h"
#include "event/session_event.h"
#include "event/sql_event.h"
#include "sql/executor/command_executor.h"
#include "sql/operator/physical_operator.h"
#include "sql/operator/string_list_physical_operator.h"
#include "sql/stmt/stmt.h"
#include "sql/parser/parse_defs.h"
#include "session/session.h"
#include "sql/expr/tuple.h"
#include "storage/field/field_meta.h"

using namespace common;

// 辅助函数：处理 SET 变量命令
RC do_set_variable(SessionEvent *session_event, SetVariableSqlNode *sql_node) {
  Session *session = session_event->session();
  const std::string &name = sql_node->name;
  const Value &val = sql_node->value;
  
  // 转换 Value 为 bool
  bool bool_value = false;
  if (val.attr_type() == AttrType::INTS) {
    bool_value = (val.get_int() != 0);
  } else if (val.attr_type() == AttrType::CHARS) {
    std::string s = val.get_string();
    if (strcasecmp(s.c_str(), "true") == 0 || strcmp(s.c_str(), "1") == 0) {
      bool_value = true;
    }
  } else {
    // 默认尝试按整型处理
    bool_value = (val.get_int() != 0);
  }

  // 使用 strcasecmp 代替 common::string_util::case_ignore_equal
  if (strcasecmp(name.c_str(), "sql_debug") == 0) {
    session->set_sql_debug(bool_value);
  } else if (strcasecmp(name.c_str(), "hash_join") == 0) {
    session->set_hash_join(bool_value);
  } else if (strcasecmp(name.c_str(), "use_cascade") == 0) {
    session->set_use_cascade(bool_value);
  } else {
    LOG_WARN("Unknown variable or read-only: %s", name.c_str());
  }
  return RC::SUCCESS;
}

RC ExecuteStage::handle_request(SQLStageEvent *sql_event)
{
  RC rc = RC::SUCCESS;
  SessionEvent *session_event = sql_event->session_event();

  // 从 sql_event 获取解析后的 SQL 节点
  ParsedSqlNode *sql_node = sql_event->sql_node().get();

  if (!sql_node) {
      LOG_WARN("SQL node is null");
      return RC::INTERNAL;
  }

  switch (sql_node->flag) {
    case SCF_SET_VARIABLE: {
      rc = do_set_variable(session_event, &sql_node->set_variable);
      break;
    }
    case SCF_SHOW_VARIABLES: {
      rc = do_show_variables(session_event, &sql_node->show_variables);
      break;
    }
    case SCF_EXIT: {
      // 这里的 API 不支持 set_is_terminate，直接返回 SUCCESS
      // MiniOB 的上层逻辑会处理连接关闭，或者 CommandExecutor 会再次处理
      rc = RC::SUCCESS; 
      break;
    }
    default: {
      // 默认处理流程
      const unique_ptr<PhysicalOperator> &physical_operator = sql_event->physical_operator();
      if (physical_operator != nullptr) {
        return handle_request_with_physical_operator(sql_event);
      }

      Stmt *stmt = sql_event->stmt();
      if (stmt != nullptr) {
        CommandExecutor command_executor;
        rc = command_executor.execute(sql_event);
        session_event->sql_result()->set_return_code(rc);
      } else {
        // 如果是 HELP 等命令，Parser 解析了但没有生成 Stmt 和 Operator
        if (sql_node->flag == SCF_HELP) {
           // Help 可以在这里处理，或者交给 default 的 CommandExecutor
        } else {
           LOG_WARN("Unknown command flag: %d", sql_node->flag);
           rc = RC::INTERNAL; // 使用 INTERNAL 替代 UNEXPECTED
        }
      }
      break;
    }
  }

  return rc;
}

RC ExecuteStage::handle_request_with_physical_operator(SQLStageEvent *sql_event)
{
  RC rc = RC::SUCCESS;

  unique_ptr<PhysicalOperator> &physical_operator = sql_event->physical_operator();
  ASSERT(physical_operator != nullptr, "physical operator should not be null");

  SqlResult *sql_result = sql_event->session_event()->sql_result();
  sql_result->set_operator(std::move(physical_operator));
  return rc;
}

// 实现 SHOW VARIABLES
RC ExecuteStage::do_show_variables(SessionEvent *session_event, ShowVariablesSqlNode *sql_node)
{
  // 1. 构造 Schema
  TupleSchema schema;
  schema.append_cell(TupleCellSpec("Variable_name"));
  schema.append_cell(TupleCellSpec("Value"));

  // 2. 创建 StringListPhysicalOperator
  auto op = std::make_unique<StringListPhysicalOperator>();
  
  // 关键修正：StringListPhysicalOperator 似乎没有 set_schema 接口
  // 我们直接将 Schema 设置到 SqlResult 中，这是最安全的做法
  session_event->sql_result()->set_tuple_schema(schema);

  // 3. 收集变量数据
  Session *session = session_event->session();
  std::map<std::string, std::string> variables;
  
  // 从 Session 获取状态
  variables["sql_debug"] = session->sql_debug_on() ? "true" : "false";
  variables["hash_join"] = session->hash_join_on() ? "true" : "false";
  variables["use_cascade"] = session->use_cascade() ? "true" : "false";

  // 4. 填充数据
  const std::string &pattern = sql_node->pattern;

  for (const auto &item : variables) {
    const std::string &key = item.first;
    const std::string &val = item.second;

    // 简单匹配逻辑 (substring match)
    // 如果 pattern 为空或者 key 包含 pattern
    if (pattern.empty() || key.find(pattern) != std::string::npos) {
      // 使用 initializer_list 插入数据
      op->append({key, val});
    }
  }

  // 5. 设置 Operator 到 Result
  session_event->sql_result()->set_operator(std::move(op));
  
  return RC::SUCCESS;
}