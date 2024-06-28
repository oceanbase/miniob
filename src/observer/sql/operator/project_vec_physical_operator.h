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

#include "sql/operator/physical_operator.h"
#include "sql/expr/expression_tuple.h"

/**
 * @brief 选择/投影物理算子(vectorized)
 * @ingroup PhysicalOperator
 */
class ProjectVecPhysicalOperator : public PhysicalOperator
{
public:
  ProjectVecPhysicalOperator() {}
  ProjectVecPhysicalOperator(std::vector<std::unique_ptr<Expression>> &&expressions);

  virtual ~ProjectVecPhysicalOperator() = default;

  PhysicalOperatorType type() const override { return PhysicalOperatorType::PROJECT_VEC; }

  RC open(Trx *trx) override;
  RC next(Chunk &chunk) override;
  RC close() override;

  RC tuple_schema(TupleSchema &schema) const override;

  std::vector<std::unique_ptr<Expression>> &expressions() { return expressions_; }

private:
  std::vector<std::unique_ptr<Expression>> expressions_;
  Chunk                                    chunk_;
};
