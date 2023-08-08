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
// Created by Longda on 2021/4/14.
//

#pragma once

#include <string>
#include <memory>
#include "common/seda/stage_event.h"
#include "sql/operator/physical_operator.h"

class SessionEvent;
class Stmt;
class ParsedSqlNode;

/**
 * @brief 与SessionEvent类似，也是处理SQL请求的事件，只是用在SQL的不同阶段
 */
class SQLStageEvent : public common::StageEvent
{
public:
  SQLStageEvent(SessionEvent *event, const std::string &sql);
  virtual ~SQLStageEvent() noexcept;

  SessionEvent *session_event() const
  {
    return session_event_;
  }

  const std::string &sql() const
  {
    return sql_;
  }
  const std::unique_ptr<ParsedSqlNode> &sql_node() const
  {
    return sql_node_;
  }
  Stmt *stmt() const
  {
    return stmt_;
  }
  std::unique_ptr<PhysicalOperator> &physical_operator()
  {
    return operator_;
  }
  const std::unique_ptr<PhysicalOperator> &physical_operator() const
  {
    return operator_;
  }

  void set_sql(const char *sql)
  {
    sql_ = sql;
  }
  void set_sql_node(std::unique_ptr<ParsedSqlNode> sql_node)
  {
    sql_node_ = std::move(sql_node);
  }
  void set_stmt(Stmt *stmt)
  {
    stmt_ = stmt;
  }
  void set_operator(std::unique_ptr<PhysicalOperator> oper)
  {
    operator_ = std::move(oper);
  }

private:
  SessionEvent *session_event_ = nullptr;
  std::string sql_;  ///< 处理的SQL语句
  std::unique_ptr<ParsedSqlNode> sql_node_;  ///< 语法解析后的SQL命令
  Stmt *stmt_ = nullptr;  ///< Resolver之后生成的数据结构
  std::unique_ptr<PhysicalOperator> operator_; ///< 生成的执行计划，也可能没有
};
