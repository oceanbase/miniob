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

#pragma once

#include <string.h>
#include <string>

#include "common/seda/stage_event.h"
#include "sql/executor/sql_result.h"
#include "event/sql_debug.h"

class Session;
class Communicator;

/**
 * @brief 表示一个SQL请求
 * 
 */
class SessionEvent : public common::StageEvent 
{
public:
  SessionEvent(Communicator *client);
  virtual ~SessionEvent();

  Communicator *get_communicator() const;
  Session *session() const;

  void set_query(const std::string &query) { query_ = query; }

  const std::string &query() const { return query_; }

  SqlResult *sql_result() { return &sql_result_; }

  SqlDebug &sql_debug() { return sql_debug_; }

private:
  Communicator *communicator_ = nullptr;  ///< 与客户端通讯的对象
  SqlResult     sql_result_;              ///< SQL执行结果
  SqlDebug      sql_debug_;               ///< SQL调试信息
  std::string   query_;                   ///< SQL语句
};
