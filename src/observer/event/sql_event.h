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

#ifndef __OBSERVER_SQL_EVENT_SQLEVENT_H__
#define __OBSERVER_SQL_EVENT_SQLEVENT_H__

#include <string>
#include "common/seda/stage_event.h"

class SessionEvent;
class Stmt;
struct Query;

class SQLStageEvent : public common::StageEvent
{
public:
  SQLStageEvent(SessionEvent *event, const std::string &sql);
  virtual ~SQLStageEvent() noexcept;

  SessionEvent *session_event() const
  {
    return session_event_;
  }

  const std::string &sql() const { return sql_; }
  Query *query() const { return query_; }
  Stmt *stmt() const { return stmt_; }

  void set_sql(const char *sql) { sql_ = sql; }
  void set_query(Query *query) { query_ = query; }
  void set_stmt(Stmt *stmt) { stmt_ = stmt; }

private:
  SessionEvent *session_event_ = nullptr;
  std::string sql_;
  Query *query_ = nullptr;
  Stmt *stmt_ = nullptr;
};

#endif  //__SRC_OBSERVER_SQL_EVENT_SQLEVENT_H__
