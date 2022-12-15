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
// Created by Wangyunlai on 2022/12/14.
//

#include "sql/optimizer/physical_plan_generator.h"
#include "sql/operator/table_get_logical_operator.h"
#include "sql/operator/table_scan_operator.h"
#include "sql/operator/predicate_logical_operator.h"
#include "sql/operator/predicate_operator.h"
#include "sql/operator/project_logical_operator.h"
#include "sql/operator/project_operator.h"

RC PhysicalPlanGenerator::create(LogicalOperator &logical_operator, std::unique_ptr<Operator> &oper)
{
  RC rc = RC::SUCCESS;

  switch (logical_operator.type()) {
    case LogicalOperatorType::TABLE_GET: {
      return create_plan(static_cast<TableGetLogicalOperator &>(logical_operator), oper);
    } break;

    case LogicalOperatorType::PREDICATE: {
      return create_plan(static_cast<PredicateLogicalOperator &>(logical_operator), oper);
    } break;

    case LogicalOperatorType::PROJECTION: {
      return create_plan(static_cast<ProjectLogicalOperator &>(logical_operator), oper);
    } break;

    default: {
      return RC::INVALID_ARGUMENT;
    }
  }
  return rc;
}

RC PhysicalPlanGenerator::create_plan(TableGetLogicalOperator &table_get_oper, std::unique_ptr<Operator> &oper)
{
  // TODO table scan or index scan
  Table *table = table_get_oper.table();
  oper = std::unique_ptr<Operator>(new TableScanOperator(table));
  return RC::SUCCESS;
}

RC PhysicalPlanGenerator::create_plan(PredicateLogicalOperator &pred_oper, std::unique_ptr<Operator> &oper)
{
  std::vector<std::unique_ptr<LogicalOperator>> &children_opers = pred_oper.children();
  ASSERT(children_opers.size() == 1, "predicate logical operator's sub oper number should be 1");

  LogicalOperator &child_oper = *children_opers.front();

  std::unique_ptr<Operator> child_phy_oper;
  RC rc = create(child_oper, child_phy_oper);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to create child operator of predicate operator. rc=%s", strrc(rc));
    return rc;
  }
  
  std::vector<std::unique_ptr<Expression>> &expressions = pred_oper.expressions();
  ASSERT(expressions.size() == 1, "predicate logical operator's children should be 1");

  std::unique_ptr<Expression> expression = std::move(expressions.front());
  oper = std::unique_ptr<Operator>(new PredicateOperator(std::move(expression)));
  oper->add_child(std::move(child_phy_oper));
  return rc;
}

RC PhysicalPlanGenerator::create_plan(ProjectLogicalOperator &project_oper, std::unique_ptr<Operator> &oper)
{
  std::vector<std::unique_ptr<LogicalOperator>> &child_opers = project_oper.children();
  ASSERT(child_opers.size() == 1, "project logical operator's child number should be 1");

  LogicalOperator *child_oper = child_opers.front().get();
  std::unique_ptr<Operator> child_phy_oper;
  RC rc = create(*child_oper, child_phy_oper);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to create project logical operator's child physical operator. rc=%s", strrc(rc));
    return rc;
  }

  ProjectOperator *project_operator = new ProjectOperator;
  const std::vector<Field> &project_fields = project_oper.fields();
  for (const Field & field : project_fields) {
    project_operator->add_projection(field.table(), field.meta());
  }
  project_operator->add_child(std::move(child_phy_oper));

  oper = std::unique_ptr<Operator>(project_operator);

  LOG_TRACE("create a project physical operator");
  return rc;
}
