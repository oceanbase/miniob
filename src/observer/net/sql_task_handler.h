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
// Created by Wangyunlai on 2024/01/10.
//

#pragma once

#include "common/rc.h"
#include "session/session_stage.h"
#include "sql/executor/execute_stage.h"
#include "sql/optimizer/optimize_stage.h"
#include "sql/parser/parse_stage.h"
#include "sql/parser/resolve_stage.h"
#include "sql/query_cache/query_cache_stage.h"

class Communicator;
class SQLStageEvent;

class SqlTaskHandler
{
public:
  SqlTaskHandler()          = default;
  virtual ~SqlTaskHandler() = default;

  /**
   * @brief 指定连接上有数据可读时就读取消息然后处理
   * 
   * @param communicator 
   * @return RC 如果返回失败，就会断开连接
   */
  RC operator()(Communicator *communicator);

  RC handle_sql(SQLStageEvent *sql_event);

private:
  SessionStage    session_stage_;
  QueryCacheStage query_cache_stage_;
  ParseStage      parse_stage_;
  ResolveStage    resolve_stage_;
  OptimizeStage   optimize_stage_;
  ExecuteStage    execute_stage_;
};