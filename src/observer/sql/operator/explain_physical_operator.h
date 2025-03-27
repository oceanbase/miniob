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
// Created by WangYunlai on 2022/12/27.
//

#pragma once

#include "sql/operator/physical_operator.h"

/**
 * @brief Explain物理算子
 * @ingroup PhysicalOperator
 */
class ExplainPhysicalOperator : public PhysicalOperator
{
public:
  ExplainPhysicalOperator()          = default;
  virtual ~ExplainPhysicalOperator() = default;

  PhysicalOperatorType type() const override { return PhysicalOperatorType::EXPLAIN; }

  OpType get_op_type() const override { return OpType::EXPLAIN; }

  double calculate_cost(LogicalProperty *prop, const vector<LogicalProperty *> &child_log_props, CostModel *cm) override
  {
    return 0.0;
  }

  RC     open(Trx *trx) override;
  RC     next() override;
  RC     next(Chunk &chunk) override;
  RC     close() override;
  Tuple *current_tuple() override;

  RC tuple_schema(TupleSchema &schema) const override
  {
    schema.append_cell("Query Plan");
    return RC::SUCCESS;
  }

private:
  void generate_physical_plan();

private:
  string         physical_plan_;
  ValueListTuple tuple_;
};
