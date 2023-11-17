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
// Created by Wangyunlai on 2023/7/12.
//

#pragma once

#include "common/rc.h"

class SQLStageEvent;
class Table;
class SqlResult;

/**
 * @brief 导入数据的执行器
 * @ingroup Executor
 */
class LoadDataExecutor
{
public:
  LoadDataExecutor()          = default;
  virtual ~LoadDataExecutor() = default;

  RC execute(SQLStageEvent *sql_event);

private:
  void load_data(Table *table, const char *file_name, SqlResult *sql_result);
};
