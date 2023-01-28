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
#include "sql/operator/delete_logical_operator.h"
#include "sql/operator/join_logical_operator.h"
#include "sql/operator/project_logical_operator.h"
#include "sql/operator/explain_logical_operator.h"
#include "sql/executor/sql_result.h"
#include "sql/stmt/stmt.h"
#include "sql/stmt/filter_stmt.h"
#include "sql/stmt/select_stmt.h"
#include "sql/stmt/delete_stmt.h"
#include "sql/stmt/explain_stmt.h"
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
  SQLStageEvent *sql_event = static_cast<SQLStageEvent *>(event);

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

  std::unique_ptr<PhysicalOperator> physical_operator;
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

RC OptimizeStage::generate_physical_plan(
    std::unique_ptr<LogicalOperator> &logical_operator, std::unique_ptr<PhysicalOperator> &physical_operator)
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
  LOG_TRACE("Enter");
  LOG_TRACE("Exit");
  return;
}

RC OptimizeStage::rewrite(std::unique_ptr<LogicalOperator> &logical_operator)
{
  RC rc = RC::SUCCESS;
  bool change_made = false;
  do {
    change_made = false;
    rc = rewriter_.rewrite(logical_operator, change_made);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to do expression rewrite on logical plan. rc=%s", strrc(rc));
      return rc;
    }
  } while (change_made);

  return rc;
}

RC OptimizeStage::create_logical_plan(Stmt *stmt, std::unique_ptr<LogicalOperator> &logical_operator)
{
  RC rc = RC::SUCCESS;
  switch (stmt->type()) {
    case StmtType::SELECT: {
      SelectStmt *select_stmt = static_cast<SelectStmt *>(stmt);
      rc = create_select_logical_plan(select_stmt, logical_operator);
    } break;

    case StmtType::DELETE: {
      DeleteStmt *delete_stmt = static_cast<DeleteStmt *>(stmt);
      rc = create_delete_logical_plan(delete_stmt, logical_operator);
    } break;

    case StmtType::EXPLAIN: {
      ExplainStmt *explain_stmt = static_cast<ExplainStmt *>(stmt);
      rc = create_explain_logical_plan(explain_stmt, logical_operator);
    } break;
    default: {
      rc = RC::UNIMPLENMENT;
    }
  }
  return rc;
}
RC OptimizeStage::create_logical_plan(SQLStageEvent *sql_event, std::unique_ptr<LogicalOperator> &logical_operator)
{
  Stmt *stmt = sql_event->stmt();
  if (nullptr == stmt) {
    return RC::UNIMPLENMENT;
  }

  return create_logical_plan(stmt, logical_operator);
}

RC OptimizeStage::create_select_logical_plan(
    SelectStmt *select_stmt, std::unique_ptr<LogicalOperator> &logical_operator)
{
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
      JoinLogicalOperator *join_oper = new JoinLogicalOperator;
      join_oper->add_child(std::move(table_oper));
      join_oper->add_child(std::move(table_get_oper));
      table_oper = std::unique_ptr<LogicalOperator>(join_oper);
    }
  }

  std::unique_ptr<LogicalOperator> predicate_oper;
  RC rc = create_predicate_logical_plan(select_stmt->filter_stmt(), predicate_oper);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to create predicate logical plan. rc=%s", strrc(rc));
    return rc;
  }

  std::unique_ptr<LogicalOperator> project_oper(new ProjectLogicalOperator(all_fields));
  if (predicate_oper) {
    predicate_oper->add_child(move(table_oper));
    project_oper->add_child(move(predicate_oper));
  } else {
    project_oper->add_child(move(table_oper));
  }

  logical_operator.swap(project_oper);
  return RC::SUCCESS;
}

RC OptimizeStage::create_predicate_logical_plan(
    FilterStmt *filter_stmt, std::unique_ptr<LogicalOperator> &logical_operator)
{
  std::vector<std::unique_ptr<Expression>> cmp_exprs;
  const std::vector<FilterUnit *> &filter_units = filter_stmt->filter_units();
  for (const FilterUnit *filter_unit : filter_units) {
    const FilterObj &filter_obj_left = filter_unit->left();
    const FilterObj &filter_obj_right = filter_unit->right();

    std::unique_ptr<Expression> left(filter_obj_left.is_attr
                                         ? static_cast<Expression *>(new FieldExpr(filter_obj_left.field))
                                         : static_cast<Expression *>(new ValueExpr(filter_obj_left.value)));

    std::unique_ptr<Expression> right(filter_obj_right.is_attr
                                          ? static_cast<Expression *>(new FieldExpr(filter_obj_right.field))
                                          : static_cast<Expression *>(new ValueExpr(filter_obj_right.value)));

    ComparisonExpr *cmp_expr = new ComparisonExpr(filter_unit->comp(), std::move(left), std::move(right));
    cmp_exprs.emplace_back(cmp_expr);
  }

  std::unique_ptr<PredicateLogicalOperator> predicate_oper;
  if (!cmp_exprs.empty()) {
    std::unique_ptr<ConjunctionExpr> conjunction_expr(new ConjunctionExpr(ConjunctionExpr::Type::AND, cmp_exprs));
    predicate_oper = std::unique_ptr<PredicateLogicalOperator>(new PredicateLogicalOperator(move(conjunction_expr)));
  }

  logical_operator = move(predicate_oper);
  return RC::SUCCESS;
}

RC OptimizeStage::create_delete_logical_plan(
    DeleteStmt *delete_stmt, std::unique_ptr<LogicalOperator> &logical_operator)
{
  Table *table = delete_stmt->table();
  FilterStmt *filter_stmt = delete_stmt->filter_stmt();
  std::vector<Field> fields;
  for (int i = table->table_meta().sys_field_num(); i < table->table_meta().field_num(); i++) {
    const FieldMeta *field_meta = table->table_meta().field(i);
    fields.push_back(Field(table, field_meta));
  }
  std::unique_ptr<LogicalOperator> table_get_oper(new TableGetLogicalOperator(table, fields));

  std::unique_ptr<LogicalOperator> predicate_oper;
  RC rc = create_predicate_logical_plan(filter_stmt, predicate_oper);
  if (rc != RC::SUCCESS) {
    return rc;
  }

  std::unique_ptr<LogicalOperator> delete_oper(new DeleteLogicalOperator(table));

  if (predicate_oper) {
    predicate_oper->add_child(move(table_get_oper));
    delete_oper->add_child(move(predicate_oper));
  } else {
    delete_oper->add_child(move(table_get_oper));
  }

  logical_operator = move(delete_oper);
  return rc;
}

RC OptimizeStage::create_explain_logical_plan(
    ExplainStmt *explain_stmt, std::unique_ptr<LogicalOperator> &logical_operator)
{
  Stmt *child_stmt = explain_stmt->child();
  std::unique_ptr<LogicalOperator> child_oper;
  RC rc = create_logical_plan(child_stmt, child_oper);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to create explain's child operator. rc=%s", strrc(rc));
    return rc;
  }

  logical_operator = std::unique_ptr<LogicalOperator>(new ExplainLogicalOperator);
  logical_operator->add_child(std::move(child_oper));
  return rc;
}
