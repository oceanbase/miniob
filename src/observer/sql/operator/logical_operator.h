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
// Created by Wangyunlai on 2022/12/07.
//

#pragma once

#include <memory>
#include <vector>

#include "sql/expr/expression.h"

enum class LogicalOperatorType {
  TABLE_GET,
  PREDICATE,
  PROJECTION,
  JOIN,
  DELETE,
  EXPLAIN,
};

/**
 * 逻辑算子描述当前执行计划要做什么
 * 可以看OptimizeStage中相关的代码
 */
class LogicalOperator {
public:
  LogicalOperator() = default;
  virtual ~LogicalOperator();

  virtual LogicalOperatorType type() const = 0;

  void add_child(std::unique_ptr<LogicalOperator> oper);
  std::vector<std::unique_ptr<LogicalOperator>> &children()
  {
    return children_;
  }
  std::vector<std::unique_ptr<Expression>> &expressions()
  {
    return expressions_;
  }

protected:
  std::vector<std::unique_ptr<LogicalOperator>> children_;
  std::vector<std::unique_ptr<Expression>> expressions_;
};
