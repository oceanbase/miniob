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
// Created by WangYunlai on 2024/05/30.
//

#include "common/log/log.h"
#include "sql/operator/hash_group_by_physical_operator.h"
#include "sql/expr/expression_tuple.h"
#include "sql/expr/composite_tuple.h"

using namespace std;
using namespace common;

HashGroupByPhysicalOperator::HashGroupByPhysicalOperator(
    vector<unique_ptr<Expression>> &&group_by_exprs, vector<Expression *> &&expressions)
    : GroupByPhysicalOperator(std::move(expressions)), group_by_exprs_(std::move(group_by_exprs))
{
}

RC HashGroupByPhysicalOperator::open(Trx *trx)
{
  ASSERT(children_.size() == 1, "group by operator only support one child, but got %d", children_.size());

  PhysicalOperator &child = *children_[0];
  RC                rc    = child.open(trx);
  if (OB_FAIL(rc)) {
    LOG_INFO("failed to open child operator. rc=%s", strrc(rc));
    return rc;
  }

  ExpressionTuple<Expression *> group_value_expression_tuple(value_expressions_);

  ValueListTuple group_by_evaluated_tuple;

  while (OB_SUCC(rc = child.next())) {
    Tuple *child_tuple = child.current_tuple();
    if (nullptr == child_tuple) {
      LOG_WARN("failed to get tuple from child operator. rc=%s", strrc(rc));
      return RC::INTERNAL;
    }

    // 找到对应的group
    GroupType *found_group = nullptr;
    rc                     = find_group(*child_tuple, found_group);
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to find group. rc=%s", strrc(rc));
      return rc;
    }

    // 计算需要做聚合的值
    group_value_expression_tuple.set_tuple(child_tuple);

    // 计算聚合值
    GroupValueType &group_value = get<1>(*found_group);
    rc = aggregate(get<0>(group_value), group_value_expression_tuple);
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to aggregate values. rc=%s", strrc(rc));
      return rc;
    }
  }

  if (RC::RECORD_EOF == rc) {
    rc = RC::SUCCESS;
  }

  if (OB_FAIL(rc)) {
    LOG_WARN("failed to get next tuple. rc=%s", strrc(rc));
    return rc;
  }

  // 得到最终聚合后的值
  for (GroupType &group : groups_) {
    GroupValueType &group_value = get<1>(group);
    rc = evaluate(group_value);
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to evaluate group value. rc=%s", strrc(rc));
      return rc;
    }
  }

  current_group_ = groups_.begin();
  first_emited_  = false;
  return rc;
}

RC HashGroupByPhysicalOperator::next()
{
  if (current_group_ == groups_.end()) {
    return RC::RECORD_EOF;
  }

  if (first_emited_) {
    ++current_group_;
  } else {
    first_emited_ = true;
  }
  if (current_group_ == groups_.end()) {
    return RC::RECORD_EOF;
  }

  return RC::SUCCESS;
}

RC HashGroupByPhysicalOperator::close()
{
  children_[0]->close();
  LOG_INFO("close group by operator");
  return RC::SUCCESS;
}

Tuple *HashGroupByPhysicalOperator::current_tuple()
{
  if (current_group_ != groups_.end()) {
    GroupValueType &group_value = get<1>(*current_group_);
    return &get<1>(group_value);
  }
  return nullptr;
}

RC HashGroupByPhysicalOperator::find_group(const Tuple &child_tuple, GroupType *&found_group)
{
  found_group = nullptr;

  RC rc = RC::SUCCESS;

  ExpressionTuple<unique_ptr<Expression>> group_by_expression_tuple(group_by_exprs_);
  ValueListTuple                          group_by_evaluated_tuple;
  group_by_expression_tuple.set_tuple(&child_tuple);
  rc = ValueListTuple::make(group_by_expression_tuple, group_by_evaluated_tuple);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to get values from expression tuple. rc=%s", strrc(rc));
    return rc;
  }

  // 找到对应的group
  for (GroupType &group : groups_) {
    int compare_result = 0;
    rc                 = group_by_evaluated_tuple.compare(get<0>(group), compare_result);
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to compare group by values. rc=%s", strrc(rc));
      return rc;
    }

    if (compare_result == 0) {
      found_group = &group;
      break;
    }
  }

  // 如果没有找到对应的group，创建一个新的group
  if (nullptr == found_group) {
    AggregatorList aggregator_list;
    create_aggregator_list(aggregator_list);

    ValueListTuple child_tuple_to_value;
    rc = ValueListTuple::make(child_tuple, child_tuple_to_value);
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to make tuple to value list. rc=%s", strrc(rc));
      return rc;
    }

    CompositeTuple composite_tuple;
    composite_tuple.add_tuple(make_unique<ValueListTuple>(std::move(child_tuple_to_value)));
    groups_.emplace_back(std::move(group_by_evaluated_tuple), 
                         GroupValueType(std::move(aggregator_list), std::move(composite_tuple)));
    found_group = &groups_.back();
  }

  return rc;
}