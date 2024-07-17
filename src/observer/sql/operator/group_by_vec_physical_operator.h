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

#include "sql/expr/aggregate_hash_table.h"
#include "sql/operator/physical_operator.h"
#include <algorithm>
#include "common/log/log.h"
#include "sql/expr/aggregate_state.h"
#include "sql/expr/expression_tuple.h"
#include "sql/expr/composite_tuple.h"

using namespace std;
using namespace common;  
/**
 * @brief Group By 物理算子(vectorized)
 * @ingroup PhysicalOperator
 */
class GroupByVecPhysicalOperator : public PhysicalOperator
{
public:
  GroupByVecPhysicalOperator(
      vector<unique_ptr<Expression>> &&group_by_exprs, vector<Expression *> &&expressions)
  {
      group_by_exprs_=std::move(group_by_exprs);
      aggregate_expressions_ = std::move(expressions);
      value_expressions_.reserve(aggregate_expressions_.size());

      ranges::for_each(aggregate_expressions_, [this](Expression *expr) {
          auto *      aggregate_expr = static_cast<AggregateExpr *>(expr);
          Expression *child_expr     = aggregate_expr->child().get();
          ASSERT(child_expr != nullptr, "aggregation expression must have a child expression");
          value_expressions_.emplace_back(child_expr);
      });

      hashtable = std::make_unique<StandardAggregateHashTable>(expressions);
      hashtable_scanner = std::make_unique<StandardAggregateHashTable::Scanner>(hashtable.get());
  }
  virtual ~GroupByVecPhysicalOperator() = default;

  PhysicalOperatorType type() const override { return PhysicalOperatorType::GROUP_BY_VEC; }

  RC open(Trx *trx) override;
  RC next(Chunk &chunk) override;
  RC close() override;

private:
  std::vector<std::unique_ptr<Expression>> group_by_exprs_;
  std::vector<Expression *> aggregate_expressions_;  /// 聚合表达式
  std::vector<Expression *> value_expressions_;      /// 计算聚合时的表达式
  Chunk                     chunk_;
  Chunk                     output_chunk_;

  std::unique_ptr<StandardAggregateHashTable> hashtable;
  std::unique_ptr<StandardAggregateHashTable::Scanner> hashtable_scanner;
};