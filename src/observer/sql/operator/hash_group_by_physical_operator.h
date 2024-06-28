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

#include "sql/operator/group_by_physical_operator.h"
#include "sql/expr/composite_tuple.h"

/**
 * @brief Group By Hash 方式物理算子
 * @ingroup PhysicalOperator
 * @details 通过 hash 的方式进行 group by 操作。当聚合函数存在 group by
 * 表达式时，默认采用这个物理算子（当前也只有这个物理算子）。 NOTE:
 * 当前并没有使用hash方式实现，而是每次使用线性查找的方式。
 */
class HashGroupByPhysicalOperator : public GroupByPhysicalOperator
{
public:
  HashGroupByPhysicalOperator(
      std::vector<std::unique_ptr<Expression>> &&group_by_exprs, std::vector<Expression *> &&expressions);

  virtual ~HashGroupByPhysicalOperator() = default;

  PhysicalOperatorType type() const override { return PhysicalOperatorType::HASH_GROUP_BY; }

  RC open(Trx *trx) override;
  RC next() override;
  RC close() override;

  Tuple *current_tuple() override;

private:
  using AggregatorList = GroupByPhysicalOperator::AggregatorList;
  using GroupValueType = GroupByPhysicalOperator::GroupValueType;
  /// 聚合出来的一组数据
  using GroupType = std::tuple<ValueListTuple, GroupValueType>;

private:
  RC find_group(const Tuple &child_tuple, GroupType *&found_group);

private:
  std::vector<std::unique_ptr<Expression>> group_by_exprs_;

  /// 一组一条数据
  /// pair的first是group by 的值列表，second是计算出来的表达式值列表
  /// TODO 改成hash/unordered_map
  std::vector<GroupType> groups_;

  std::vector<GroupType>::iterator current_group_;
  bool                             first_emited_ = false;  /// 第一条数据是否已经输出
};