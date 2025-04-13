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
// Created by WangYunlai on 2023/4/25.
//

#pragma once

#include "sql/operator/logical_operator.h"
#include "sql/parser/parse_defs.h"

/**
 * @brief 插入逻辑算子
 * @ingroup LogicalOperator
 */
class InsertLogicalOperator : public LogicalOperator
{
public:
  InsertLogicalOperator(Table *table, vector<Value> values);
  virtual ~InsertLogicalOperator() = default;

  LogicalOperatorType type() const override { return LogicalOperatorType::INSERT; }

  OpType get_op_type() const override { return OpType::LOGICALINSERT; }

  Table               *table() const { return table_; }
  const vector<Value> &values() const { return values_; }
  vector<Value>       &values() { return values_; }

private:
  Table        *table_ = nullptr;
  vector<Value> values_;
};
