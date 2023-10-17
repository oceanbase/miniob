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

#include "common/seda/stage.h"
#include "sql/query_cache/query_cache_stage.h"
#include "sql/parser/parse_stage.h"
#include "sql/parser/resolve_stage.h"
#include "sql/optimizer/optimize_stage.h"
#include "sql/executor/execute_stage.h"

/**
 * @brief SEDA处理的stage
 * @defgroup SQLStage
 * @details 收到的客户端请求会放在SEDA框架中处理，每个stage都是一个处理阶段。
 * 当前的处理流程可以通过observer.ini配置文件查看。
 * seda::stage使用说明：
 * 这里利用seda的线程池与调度。stage是一个事件处理的几个阶段。
 * 目前包括session,parse,execution和storage
 * 每个stage使用handleEvent函数处理任务，并且使用StageEvent::pushCallback注册回调函数。
 * 这时当调用StageEvent::done(Immediate)时，就会调用该事件注册的回调函数，如果没有回调函数，就会释放自己。
 */

/**
 * @brief SQL处理的session阶段，也是第一个阶段
 * @ingroup SQLStage
 */
class SessionStage : public common::Stage 
{
public:
  virtual ~SessionStage();
  static Stage *make_stage(const std::string &tag);

protected:
  // common function
  SessionStage(const char *tag);
  bool set_properties() override;

  bool initialize() override;
  void cleanup() override;
  void handle_event(common::StageEvent *event) override;

protected:
  void handle_request(common::StageEvent *event);
  RC   handle_sql(SQLStageEvent *sql_event);

private:
  QueryCacheStage query_cache_stage_;
  ParseStage      parse_stage_;
  ResolveStage    resolve_stage_;
  OptimizeStage   optimize_stage_;
  ExecuteStage    execute_stage_;
};
