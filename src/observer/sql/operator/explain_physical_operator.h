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

  RC     open(Trx *trx) override;
  RC     next() override;
  RC     close() override;
  Tuple *current_tuple() override;

private:
  void to_string(std::ostream &os, PhysicalOperator *oper, int level, bool last_child, std::vector<bool> &ends);

private:
  std::string    physical_plan_;
  ValueListTuple tuple_;
};
