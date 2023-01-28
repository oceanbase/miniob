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
// Created by Longda on 2021/4/13.
//

#pragma once

#include <string.h>
#include <string>

#include "common/seda/stage_event.h"
#include "sql/executor/sql_result.h"

class Session;
class Communicator;
class SqlResult;

class SessionEvent : public common::StageEvent {
public:
  SessionEvent(Communicator *client);
  virtual ~SessionEvent();

  Communicator *get_communicator() const;
  Session *session() const;

  void set_query(const std::string &query)
  {
    query_ = query;
  }
  void set_sql_result(SqlResult *result)
  {
    sql_result_ = result;
  }
  const std::string &query() const
  {
    return query_;
  }
  SqlResult *sql_result() const
  {
    return sql_result_;
  }

  const char *get_response() const;
  void set_response(const char *response);
  void set_response(const char *response, int len);
  void set_response(std::string &&response);
  int get_response_len() const;
  const char *get_request_buf();  // TODO remove me

private:
  Communicator *communicator_ = nullptr;
  SqlResult *sql_result_ = nullptr;

  std::string query_;
  std::string response_;
};
