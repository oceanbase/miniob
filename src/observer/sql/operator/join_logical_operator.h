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
// Created by Wangyunlai on 2022/12/07
//

#pragma once

#include "sql/operator/logical_operator.h"

/**
 * @brief 连接算子
 * @ingroup LogicalOperator
 * @details 连接算子，用于连接两个表。对应的物理算子或者实现，可能有NestedLoopJoin，HashJoin等等。
 */
class JoinLogicalOperator : public LogicalOperator
{
public:
  JoinLogicalOperator()          = default;
  virtual ~JoinLogicalOperator() = default;

  LogicalOperatorType type() const override { return LogicalOperatorType::JOIN; }

  OpType get_op_type() const override { return OpType::LOGICALINNERJOIN; }

  unique_ptr<LogicalProperty> find_log_prop(const vector<LogicalProperty *> &log_props)
  {
    if (log_props.size() != 2) {
      return nullptr;
    }

    LogicalProperty *left_log_prop  = log_props[0];
    LogicalProperty *right_log_prop = log_props[1];
    int              card           = left_log_prop->get_card() * right_log_prop->get_card();
    for (auto &predicate : join_predicates_) {
      if (predicate->type() != ExprType::COMPARISON) {
        continue;
      }
      auto  pred_expr = dynamic_cast<ComparisonExpr *>(predicate.get());
      auto &left      = pred_expr->left();
      auto &right     = pred_expr->right();
      if (pred_expr->comp() == CompOp::EQUAL_TO && left->type() == ExprType::FIELD &&
          right->type() == ExprType::FIELD) {
        card /= std::max(std::max(left_log_prop->get_card(), right_log_prop->get_card()), 1);
      }
    }
    return make_unique<LogicalProperty>(card);
  }

private:
  LogicalOperator                    *predicate_op_ = nullptr;
  std::vector<unique_ptr<Expression>> join_predicates_;
};
