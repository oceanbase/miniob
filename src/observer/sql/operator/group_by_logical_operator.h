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
// Created by WangYunlai on 2024/05/29.
//

#pragma once

#include "sql/operator/logical_operator.h"

class GroupByLogicalOperator : public LogicalOperator
{
public:
  GroupByLogicalOperator(
      std::vector<std::unique_ptr<Expression>> &&group_by_exprs, std::vector<Expression *> &&expressions);

  virtual ~GroupByLogicalOperator() = default;

  LogicalOperatorType type() const override { return LogicalOperatorType::GROUP_BY; }

  auto &group_by_expressions() { return group_by_expressions_; }
  auto &aggregate_expressions() { return aggregate_expressions_; }

private:
  std::vector<std::unique_ptr<Expression>> group_by_expressions_;
  std::vector<Expression *>                aggregate_expressions_;  ///< 输出的表达式，可能包含聚合函数
};