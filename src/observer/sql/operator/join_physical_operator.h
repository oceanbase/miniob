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
// Created by WangYunlai on 2021/6/10.
//

#pragma once

#include "sql/parser/parse.h"
#include "sql/operator/physical_operator.h"
#include "rc.h"

// TODO fixme
class NestedLoopJoinPhysicalOperator : public PhysicalOperator
{
public:
  NestedLoopJoinPhysicalOperator();
  virtual ~NestedLoopJoinPhysicalOperator() = default;

  PhysicalOperatorType type() const override { return PhysicalOperatorType::NESTED_LOOP_JOIN; }
  
  RC open() override;
  RC next() override;
  RC close() override;
  Tuple *current_tuple() override;

private:
  RC left_next();
  RC right_next();
  
private:
  PhysicalOperator *left_ = nullptr;
  PhysicalOperator *right_ = nullptr;
  Tuple *left_tuple_ = nullptr;
  Tuple *right_tuple_ = nullptr;
  JoinedTuple joined_tuple_;
  bool round_done_ = true;
  bool right_closed_ = true;
};
