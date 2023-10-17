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
#include "sql/operator/string_list_physical_operator.h"
#include "event/sql_event.h"
#include "event/session_event.h"
#include "sql/executor/sql_result.h"
#include "session/session.h"

/**
 * @brief Help语句执行器
 * @ingroup Executor
 */
class HelpExecutor
{
public:
  HelpExecutor() = default;
  virtual ~HelpExecutor() = default;

  RC execute(SQLStageEvent *sql_event)
  {
    const char *strings[] = {
        "show tables;",
        "desc `table name`;",
        "create table `table name` (`column name` `column type`, ...);",
        "create index `index name` on `table` (`column`);",
        "insert into `table` values(`value1`,`value2`);",
        "update `table` set column=value [where `column`=`value`];",
        "delete from `table` [where `column`=`value`];",
        "select [ * | `columns` ] from `table`;"
      };

    auto oper = new StringListPhysicalOperator();
    for (size_t i = 0; i < sizeof(strings) / sizeof(strings[0]); i++) {
      oper->append(strings[i]);
    }

    SqlResult *sql_result = sql_event->session_event()->sql_result();

    TupleSchema schema;
    schema.append_cell("Commands");

    sql_result->set_tuple_schema(schema);
    sql_result->set_operator(std::unique_ptr<PhysicalOperator>(oper));

    return RC::SUCCESS;
  }
};