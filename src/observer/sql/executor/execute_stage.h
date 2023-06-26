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
#include "sql/parser/parse.h"
#include "common/rc.h"

class SQLStageEvent;
class SessionEvent;
class SelectStmt;

/**
 * @brief 执行SQL语句的Stage，包括DML和DDL
 * @ingroup SQLStage
 * @details 根据前面阶段生成的结果，有些语句会生成执行计划，有些不会。
 * 整体上分为两类，带执行计划的，或者 @class CommandExecutor 可以直接执行的。
 */
class ExecuteStage : public common::Stage 
{
public:
  virtual ~ExecuteStage();
  static Stage *make_stage(const std::string &tag);

protected:
  // common function
  ExecuteStage(const char *tag);
  bool set_properties() override;

  bool initialize() override;
  void cleanup() override;
  void handle_event(common::StageEvent *event) override;

  RC handle_request(common::StageEvent *event);
  RC handle_request_with_physical_operator(SQLStageEvent *sql_event);

private:
  Stage *default_storage_stage_ = nullptr;
  Stage *mem_storage_stage_ = nullptr;
};
