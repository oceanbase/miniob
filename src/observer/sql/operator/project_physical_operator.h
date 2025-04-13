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
// Created by WangYunlai on 2022/07/01.
//

#pragma once

#include "sql/operator/physical_operator.h"
#include "sql/expr/expression_tuple.h"

/**
 * @brief 选择/投影物理算子
 * @ingroup PhysicalOperator
 */
class ProjectPhysicalOperator : public PhysicalOperator
{
public:
  ProjectPhysicalOperator(vector<unique_ptr<Expression>> &&expressions);

  virtual ~ProjectPhysicalOperator() = default;

  PhysicalOperatorType type() const override { return PhysicalOperatorType::PROJECT; }
  OpType               get_op_type() const override { return OpType::PROJECTION; }

  virtual double calculate_cost(
      LogicalProperty *prop, const vector<LogicalProperty *> &child_log_props, CostModel *cm) override
  {
    return (cm->cpu_op()) * prop->get_card();
  }

  RC open(Trx *trx) override;
  RC next() override;
  RC close() override;

  int cell_num() const { return tuple_.cell_num(); }

  Tuple *current_tuple() override;

  RC tuple_schema(TupleSchema &schema) const override;

private:
  vector<unique_ptr<Expression>>          expressions_;
  ExpressionTuple<unique_ptr<Expression>> tuple_;
};
