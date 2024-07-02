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
// Created by WangYunlai on 2024/06/11.
//
#include <algorithm>

#include "common/log/log.h"
#include "sql/operator/group_by_physical_operator.h"
#include "sql/expr/expression_tuple.h"
#include "sql/expr/composite_tuple.h"

using namespace std;
using namespace common;

GroupByPhysicalOperator::GroupByPhysicalOperator(vector<Expression *> &&expressions)
{
  aggregate_expressions_ = std::move(expressions);
  value_expressions_.reserve(aggregate_expressions_.size());
  ranges::for_each(aggregate_expressions_, [this](Expression *expr) {
    auto       *aggregate_expr = static_cast<AggregateExpr *>(expr);
    Expression *child_expr     = aggregate_expr->child().get();
    ASSERT(child_expr != nullptr, "aggregate expression must have a child expression");
    value_expressions_.emplace_back(child_expr);
  });
}

void GroupByPhysicalOperator::create_aggregator_list(AggregatorList &aggregator_list)
{
  aggregator_list.clear();
  aggregator_list.reserve(aggregate_expressions_.size());
  ranges::for_each(aggregate_expressions_, [&aggregator_list](Expression *expr) {
    auto *aggregate_expr = static_cast<AggregateExpr *>(expr);
    aggregator_list.emplace_back(aggregate_expr->create_aggregator());
  });
}

RC GroupByPhysicalOperator::aggregate(AggregatorList &aggregator_list, const Tuple &tuple)
{
  ASSERT(static_cast<int>(aggregator_list.size()) == tuple.cell_num(), 
         "aggregator list size must be equal to tuple size. aggregator num: %d, tuple num: %d",
         aggregator_list.size(), tuple.cell_num());

  RC        rc = RC::SUCCESS;
  Value     value;
  const int size = static_cast<int>(aggregator_list.size());
  for (int i = 0; i < size; i++) {
    Aggregator *aggregator = aggregator_list[i].get();

    rc = tuple.cell_at(i, value);
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to get value from expression. rc=%s", strrc(rc));
      return rc;
    }

    rc = aggregator->accumulate(value);
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to accumulate value. rc=%s", strrc(rc));
      return rc;
    }
  }

  return rc;
}

RC GroupByPhysicalOperator::evaluate(GroupValueType &group_value)
{
  RC rc = RC::SUCCESS;

  vector<TupleCellSpec> aggregator_names;
  for (Expression *expr : aggregate_expressions_) {
    aggregator_names.emplace_back(expr->name());
  }

  AggregatorList &aggregators           = get<0>(group_value);
  CompositeTuple &composite_value_tuple = get<1>(group_value);

  ValueListTuple evaluated_tuple;
  vector<Value>  values;
  for (unique_ptr<Aggregator> &aggregator : aggregators) {
    Value value;
    rc = aggregator->evaluate(value);
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to evaluate aggregator. rc=%s", strrc(rc));
      return rc;
    }
    values.emplace_back(value);
  }

  evaluated_tuple.set_cells(values);
  evaluated_tuple.set_names(aggregator_names);

  composite_value_tuple.add_tuple(make_unique<ValueListTuple>(std::move(evaluated_tuple)));

  return rc;
}