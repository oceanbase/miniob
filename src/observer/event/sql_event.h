/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
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
class Command;

class SQLStageEvent : public common::StageEvent {
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
  const std::unique_ptr<Command> &command() const
  {
    return command_;
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
  void set_command(std::unique_ptr<Command> cmd)
  {
    command_ = std::move(cmd);
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
  std::string sql_;
  std::unique_ptr<Command> command_;
  Stmt *stmt_ = nullptr;
  std::unique_ptr<PhysicalOperator> operator_;
};
