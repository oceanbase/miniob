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
// Created by WangYunlai on 2024/06/11.
//

#pragma once

#include "sql/operator/group_by_physical_operator.h"

/**
 * @brief 没有 group by 表达式的 group by 物理算子
 * @ingroup PhysicalOperator
 */
class ScalarGroupByPhysicalOperator : public GroupByPhysicalOperator
{
public:
  ScalarGroupByPhysicalOperator(std::vector<Expression *> &&expressions);
  virtual ~ScalarGroupByPhysicalOperator() = default;

  PhysicalOperatorType type() const override { return PhysicalOperatorType::SCALAR_GROUP_BY; }

  RC open(Trx *trx) override;
  RC next() override;
  RC close() override;

  Tuple *current_tuple() override;

private:
  std::unique_ptr<GroupValueType> group_value_;
  bool                            emitted_ = false;  /// 标识是否已经输出过
};