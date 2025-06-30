/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "common/log/log.h"
#include "common/lang/ranges.h"
#include "sql/operator/aggregate_vec_physical_operator.h"
#include "sql/expr/aggregate_state.h"
#include "sql/expr/expression_tuple.h"
#include "sql/expr/composite_tuple.h"

using namespace common;

AggregateVecPhysicalOperator::AggregateVecPhysicalOperator(vector<Expression *> &&expressions)
{
  aggregate_expressions_ = std::move(expressions);
  value_expressions_.reserve(aggregate_expressions_.size());

  ranges::for_each(aggregate_expressions_, [this](Expression *expr) {
    auto *      aggregate_expr = static_cast<AggregateExpr *>(expr);
    Expression *child_expr     = aggregate_expr->child().get();
    ASSERT(child_expr != nullptr, "aggregation expression must have a child expression");
    value_expressions_.emplace_back(child_expr);
  });

  for (size_t i = 0; i < aggregate_expressions_.size(); i++) {
    auto &expr = aggregate_expressions_[i];
    ASSERT(expr->type() == ExprType::AGGREGATION, "expected an aggregation expression");
    auto *aggregate_expr = static_cast<AggregateExpr *>(expr);
    void *state_ptr = create_aggregate_state(aggregate_expr->aggregate_type(), aggregate_expr->child()->value_type());
    ASSERT(state_ptr != nullptr, "failed to create aggregate state");
    aggr_values_.insert(state_ptr);
    output_chunk_.add_column(make_unique<Column>(aggregate_expr->value_type(), aggregate_expr->value_length()), i);
  }
}

RC AggregateVecPhysicalOperator::open(Trx *trx)
{
  ASSERT(children_.size() == 1, "group by operator only support one child, but got %d", children_.size());

  PhysicalOperator &child = *children_[0];
  RC                rc    = child.open(trx);
  if (OB_FAIL(rc)) {
    LOG_INFO("failed to open child operator. rc=%s", strrc(rc));
    return rc;
  }

  while (OB_SUCC(rc = child.next(chunk_))) {
    for (size_t aggr_idx = 0; aggr_idx < aggregate_expressions_.size(); aggr_idx++) {
      Column column;
      value_expressions_[aggr_idx]->get_column(chunk_, column);
      ASSERT(aggregate_expressions_[aggr_idx]->type() == ExprType::AGGREGATION, "expect aggregate expression");
      auto *aggregate_expr = static_cast<AggregateExpr *>(aggregate_expressions_[aggr_idx]);
      rc = aggregate_state_update_by_column(aggr_values_.at(aggr_idx), aggregate_expr->aggregate_type(), aggregate_expr->child()->value_type(), column);
      if (OB_FAIL(rc)) {
        LOG_INFO("failed to update aggregate state. rc=%s", strrc(rc));
        return rc;
      }
    }
  }

  if (rc == RC::RECORD_EOF) {
    rc = RC::SUCCESS;
  }

  return rc;
}

template <class STATE, typename T>
void AggregateVecPhysicalOperator::update_aggregate_state(void *state, const Column &column)
{
  STATE *state_ptr = reinterpret_cast<STATE *>(state);
  T *    data      = (T *)column.data();
  state_ptr->update(data, column.count());
}

RC AggregateVecPhysicalOperator::next(Chunk &chunk)
{
  if (outputed_) {
    return RC::RECORD_EOF;
  }
  for (size_t i = 0; i < aggr_values_.size(); i++) {
    auto pos = i;
    ASSERT(aggregate_expressions_[pos]->type() == ExprType::AGGREGATION, "expect aggregation expression");
    auto *aggregate_expr = static_cast<AggregateExpr *>(aggregate_expressions_[pos]);
    RC rc = finialize_aggregate_state(aggr_values_.at(pos), aggregate_expr->aggregate_type(), aggregate_expr->child()->value_type(), output_chunk_.column(i));
    if (OB_FAIL(rc)) {
      LOG_INFO("failed to finialize aggregate state. rc=%s", strrc(rc));
      return rc;
    }
  }

  chunk.reference(output_chunk_);
  outputed_ = true;

  return RC::SUCCESS;
}

RC AggregateVecPhysicalOperator::close()
{
  children_[0]->close();
  LOG_INFO("close group by operator");
  return RC::SUCCESS;
}
