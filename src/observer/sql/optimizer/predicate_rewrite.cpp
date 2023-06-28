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
// Created by Wangyunlai on 2022/12/29.
//

#include "sql/optimizer/predicate_rewrite.h"
#include "sql/operator/logical_operator.h"

RC PredicateRewriteRule::rewrite(std::unique_ptr<LogicalOperator> &oper, bool &change_made)
{
  std::vector<std::unique_ptr<LogicalOperator>> &child_opers = oper->children();
  if (child_opers.size() != 1) {
    return RC::SUCCESS;
  }

  auto &child_oper = child_opers.front();
  if (child_oper->type() != LogicalOperatorType::PREDICATE) {
    return RC::SUCCESS;
  }

  std::vector<std::unique_ptr<Expression>> &expressions = child_oper->expressions();
  if (expressions.size() != 1) {
    return RC::SUCCESS;
  }

  std::unique_ptr<Expression> &expr = expressions.front();
  if (expr->type() != ExprType::VALUE) {
    return RC::SUCCESS;
  }

  // 如果仅有的一个子节点是predicate
  // 并且这个子节点可以判断为恒为TRUE，那么可以省略这个子节点，并把他的子节点们（就是孙子节点）接管过来
  // 如果可以判断恒为false，那么就可以删除子节点
  auto value_expr = static_cast<ValueExpr *>(expr.get());
  bool bool_value = value_expr->get_value().get_boolean();
  if (true == bool_value) {
    std::vector<std::unique_ptr<LogicalOperator>> grand_child_opers;
    grand_child_opers.swap(child_oper->children());
    child_opers.clear();
    for (auto &grand_child_oper : grand_child_opers) {
      oper->add_child(std::move(grand_child_oper));
    }
  } else {
    child_opers.clear();
  }

  change_made = true;
  return RC::SUCCESS;
}
