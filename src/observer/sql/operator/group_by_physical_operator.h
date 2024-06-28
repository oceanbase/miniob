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

#include "sql/operator/physical_operator.h"
#include "sql/expr/composite_tuple.h"

/**
 * @brief Group By 物理算子基类
 * @ingroup PhysicalOperator
 */
class GroupByPhysicalOperator : public PhysicalOperator
{
public:
  GroupByPhysicalOperator(std::vector<Expression *> &&expressions);
  virtual ~GroupByPhysicalOperator() = default;

protected:
  using AggregatorList = std::vector<std::unique_ptr<Aggregator>>;
  /**
   * @brief 聚合出来的一组数据
   * @details
   * 第一个参数是聚合函数列表，比如需要计算 sum(a), avg(b), count(c)。
   * 第二个参数是聚合的最终结果，它也包含两个元素，第一个是缓存下来的元组，第二个是聚合函数计算的结果。
   * 第二个参数中，之所以要缓存下来一个元组，是要解决这个问题：
   * select a, b, sum(a) from t group by a;
   * 我们需要知道b的值是什么，虽然它不确定。
   */
  using GroupValueType = std::tuple<AggregatorList, CompositeTuple>;

protected:
  void create_aggregator_list(AggregatorList &aggregator_list);

  /// @brief 聚合一条记录
  /// @param aggregator_list 需要执行聚合运算的列表
  /// @param tuple 执行聚合运算的一条记录
  RC aggregate(AggregatorList &aggregator_list, const Tuple &tuple);

  /// @brief 所有tuple聚合结束后，运算最终结果
  RC evaluate(GroupValueType &group_value);

protected:
  std::vector<Expression *> aggregate_expressions_;  /// 聚合表达式
  std::vector<Expression *> value_expressions_;      /// 计算聚合时的表达式
};