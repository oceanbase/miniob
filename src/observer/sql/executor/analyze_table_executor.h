/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#pragma once

#include "common/sys/rc.h"

class SQLStageEvent;
class RecordScanner;

/**
 * @brief 分析表的执行器(analyze table)
 * @ingroup Executor
 */
class AnalyzeTableExecutor
{
public:
  AnalyzeTableExecutor() = default;
  virtual ~AnalyzeTableExecutor();

  RC execute(SQLStageEvent *sql_event);

private:
  RecordScanner *scanner_ = nullptr;
};
