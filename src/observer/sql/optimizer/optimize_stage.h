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

#include <memory>

#include "rc.h"
#include "common/seda/stage.h"
#include "sql/operator/logical_operator.h"
#include "sql/operator/physical_operator.h"
#include "sql/optimizer/physical_plan_generator.h"
#include "sql/optimizer/rewriter.h"

class SQLStageEvent;
class LogicalOperator;
class Stmt;
class SelectStmt;
class DeleteStmt;
class FilterStmt;
class ExplainStmt;

class OptimizeStage : public common::Stage {
public:
  ~OptimizeStage();
  static Stage *make_stage(const std::string &tag);

protected:
  // common function
  OptimizeStage(const char *tag);
  bool set_properties();

  bool initialize();
  void cleanup();
  void handle_event(common::StageEvent *event);
  void callback_event(common::StageEvent *event, common::CallbackContext *context);

private:
  RC handle_request(SQLStageEvent *event);

  RC create_logical_plan(SQLStageEvent *sql_event, std::unique_ptr<LogicalOperator> &logical_operator);
  RC create_logical_plan(Stmt *stmt, std::unique_ptr<LogicalOperator> &logical_operator);
  RC create_select_logical_plan(SelectStmt *select_stmt, std::unique_ptr<LogicalOperator> &logical_operator);
  RC create_predicate_logical_plan(FilterStmt *filter_stmt, std::unique_ptr<LogicalOperator> &logical_operator);
  RC create_delete_logical_plan(DeleteStmt *delete_stmt, std::unique_ptr<LogicalOperator> &logical_operator);
  RC create_explain_logical_plan(ExplainStmt *explain_stmt, std::unique_ptr<LogicalOperator> &logical_operator);

  RC rewrite(std::unique_ptr<LogicalOperator> &logical_operator);
  RC optimize(std::unique_ptr<LogicalOperator> &logical_operator);
  RC generate_physical_plan(
      std::unique_ptr<LogicalOperator> &logical_operator, std::unique_ptr<PhysicalOperator> &physical_operator);

private:
  Stage *execute_stage_ = nullptr;
  PhysicalPlanGenerator physical_plan_generator_;
  Rewriter rewriter_;
};
