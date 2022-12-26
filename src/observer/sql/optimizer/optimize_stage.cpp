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

#include <string.h>
#include <string>

#include "optimize_stage.h"

#include "common/conf/ini.h"
#include "common/io/io.h"
#include "common/lang/string.h"
#include "common/log/log.h"
#include "common/seda/timer_stage.h"
#include "sql/expr/expression.h"
#include "sql/operator/logical_operator.h"
#include "sql/operator/project_logical_operator.h"
#include "sql/operator/predicate_logical_operator.h"
#include "sql/operator/table_get_logical_operator.h"
#include "sql/operator/join_logical_operator.h"
#include "sql/operator/project_logical_operator.h"
#include "sql/executor/sql_result.h"
#include "sql/stmt/stmt.h"
#include "sql/stmt/filter_stmt.h"
#include "sql/stmt/select_stmt.h"
#include "event/sql_event.h"
#include "event/session_event.h"

using namespace std;
using namespace common;

//! Constructor
OptimizeStage::OptimizeStage(const char *tag) : Stage(tag)
{}

//! Destructor
OptimizeStage::~OptimizeStage()
{}

//! Parse properties, instantiate a stage object
Stage *OptimizeStage::make_stage(const std::string &tag)
{
  OptimizeStage *stage = new (std::nothrow) OptimizeStage(tag.c_str());
  if (stage == nullptr) {
    LOG_ERROR("new OptimizeStage failed");
    return nullptr;
  }
  stage->set_properties();
  return stage;
}

//! Set properties for this object set in stage specific properties
bool OptimizeStage::set_properties()
{
  //  std::string stageNameStr(stage_name_);
  //  std::map<std::string, std::string> section = g_properties()->get(
  //    stageNameStr);
  //
  //  std::map<std::string, std::string>::iterator it;
  //
  //  std::string key;

  return true;
}

//! Initialize stage params and validate outputs
bool OptimizeStage::initialize()
{
  LOG_TRACE("Enter");

  std::list<Stage *>::iterator stgp = next_stage_list_.begin();
  execute_stage_ = *(stgp++);

  LOG_TRACE("Exit");
  return true;
}

//! Cleanup after disconnection
void OptimizeStage::cleanup()
{
  LOG_TRACE("Enter");

  LOG_TRACE("Exit");
}

void OptimizeStage::handle_event(StageEvent *event)
{
  LOG_TRACE("Enter");
  SQLStageEvent *sql_event = static_cast<SQLStageEvent*>(event);

  RC rc = handle_request(sql_event);
  if (rc != RC::UNIMPLENMENT && rc != RC::SUCCESS) {
    SqlResult *sql_result = new SqlResult;
    sql_result->set_return_code(rc);
    sql_event->session_event()->set_sql_result(sql_result);    
  } else {
      execute_stage_->handle_event(event);
  }
  LOG_TRACE("Exit");

}
RC OptimizeStage::handle_request(SQLStageEvent *sql_event)
{
  std::unique_ptr<LogicalOperator> logical_operator;
  RC rc = create_logical_plan(sql_event, logical_operator);
  if (rc != RC::SUCCESS) {
    if (rc != RC::UNIMPLENMENT) {
      LOG_WARN("failed to create logical plan. rc=%s", strrc(rc));
    }
    return rc;
  }
    
  rc = rewrite(logical_operator);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to rewrite plan. rc=%s", strrc(rc));
    return rc;
  }

  rc = optimize(logical_operator);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to optimize plan. rc=%s", strrc(rc));
    return rc;
  }

  std::unique_ptr<Operator> physical_operator;
  rc = generate_physical_plan(logical_operator, physical_operator);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to generate physical plan. rc=%s", strrc(rc));
    return rc;
  }

  sql_event->set_operator(std::move(physical_operator));

  return rc;
}

RC OptimizeStage::optimize(std::unique_ptr<LogicalOperator> &oper)
{
  // do nothing
  return RC::SUCCESS;
}

RC OptimizeStage::generate_physical_plan(std::unique_ptr<LogicalOperator> &logical_operator,
                                         std::unique_ptr<Operator> &physical_operator)
{
  RC rc = RC::SUCCESS;
  rc = physical_plan_generator_.create(*logical_operator, physical_operator);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to create physical operator. rc=%s", strrc(rc));
  }
  return rc;
}

void OptimizeStage::callback_event(StageEvent *event, CallbackContext *context)
{
  LOG_TRACE("Enter\n");
  LOG_TRACE("Exit\n");
  return;
}

RC OptimizeStage::rewrite(std::unique_ptr<LogicalOperator> &logical_operator)
{
  RC rc = RC::SUCCESS;
  bool change_made = false;
  do {
    change_made = false;
    rc = expression_rewriter_.rewrite(logical_operator, change_made);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to do expression rewrite on logical plan. rc=%s", strrc(rc));
      return rc;
    }
  } while (change_made);

  return rc;
}

RC OptimizeStage::create_logical_plan(SQLStageEvent *sql_event, std::unique_ptr<LogicalOperator> & logical_operator)
{
  Stmt *stmt = sql_event->stmt();
  if (nullptr == stmt || stmt->type() != StmtType::SELECT) {
    return RC::UNIMPLENMENT;
  }

  SelectStmt *select_stmt = static_cast<SelectStmt *>(stmt);

  std::unique_ptr<LogicalOperator> table_oper(nullptr);

  const std::vector<Table *> &tables = select_stmt->tables();
  const std::vector<Field> &all_fields = select_stmt->query_fields();
  for (Table *table : tables) {
    std::vector<Field> fields;
    for (const Field &field : all_fields) {
      if (0 == strcmp(field.table_name(), table->name())) {
        fields.push_back(field);
      }
    }
    
    std::unique_ptr<LogicalOperator> table_get_oper(new TableGetLogicalOperator(table, fields));
    if (table_oper == nullptr) {
      table_oper = std::move(table_get_oper);
    } else {
      JoinLogicalOperator * join_oper = new JoinLogicalOperator;
      join_oper->add_child(std::move(table_oper));
      join_oper->add_child(std::move(table_get_oper));
      table_oper = std::unique_ptr<LogicalOperator>(join_oper);
    }
  }

  std::vector<std::unique_ptr<Expression>> cmp_exprs;
  FilterStmt *filter_stmt = select_stmt->filter_stmt();
  const std::vector<FilterUnit *> &filter_units = filter_stmt->filter_units();
  for (const FilterUnit * filter_unit : filter_units) {
    const FilterObj &filter_obj_left = filter_unit->left();
    const FilterObj &filter_obj_right = filter_unit->right();

    std::unique_ptr<Expression> left(
        filter_obj_left.is_attr ?
          static_cast<Expression *>(new FieldExpr(filter_obj_left.field)) :
          static_cast<Expression *>(new ValueExpr(filter_obj_left.value)));

    std::unique_ptr<Expression> right(
        filter_obj_right.is_attr ?
          static_cast<Expression *>(new FieldExpr(filter_obj_right.field)) :
          static_cast<Expression *>(new ValueExpr(filter_obj_right.value)));
    
    ComparisonExpr *cmp_expr = new ComparisonExpr(filter_unit->comp(), std::move(left), std::move(right));
    cmp_exprs.emplace_back(cmp_expr);
  }

  std::unique_ptr<PredicateLogicalOperator> predicate_oper;
  if (!cmp_exprs.empty()) {
    std::unique_ptr<ConjunctionExpr> conjunction_expr(new ConjunctionExpr(ConjunctionExpr::Type::AND, cmp_exprs));
    predicate_oper = std::unique_ptr<PredicateLogicalOperator>(new PredicateLogicalOperator(move(conjunction_expr)));
    predicate_oper->add_child(move(table_oper));
  }
  
  std::unique_ptr<LogicalOperator> project_oper(new ProjectLogicalOperator(all_fields));
  if (predicate_oper) {
    project_oper->add_child(move(predicate_oper));
  } else {
    project_oper->add_child(move(table_oper));
  }

  logical_operator.swap(project_oper);
  return RC::SUCCESS;
}
