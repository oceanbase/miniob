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
#include "sql/executor/sql_result.h"
#include "sql/operator/string_list_physical_operator.h"
#include "storage/db/db.h"

/**
 * @brief 显示所有表的执行器
 * @ingroup Executor
 * @note 与CreateIndex类似，不处理并发
 */
class ShowTablesExecutor
{
public:
  ShowTablesExecutor()          = default;
  virtual ~ShowTablesExecutor() = default;

  RC execute(SQLStageEvent *sql_event)
  {
    SqlResult    *sql_result    = sql_event->session_event()->sql_result();
    SessionEvent *session_event = sql_event->session_event();

    Db *db = session_event->session()->get_current_db();

    std::vector<std::string> all_tables;
    db->all_tables(all_tables);

    TupleSchema tuple_schema;
    tuple_schema.append_cell(TupleCellSpec("", "Tables_in_SYS", "Tables_in_SYS"));
    sql_result->set_tuple_schema(tuple_schema);

    auto oper = new StringListPhysicalOperator;
    for (const std::string &s : all_tables) {
      oper->append(s);
    }

    sql_result->set_operator(std::unique_ptr<PhysicalOperator>(oper));
    return RC::SUCCESS;
  }
};