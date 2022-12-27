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
// Created by WangYunlai on 2022/6/7.
//

#pragma once

#include <vector>
#include <memory>

#include "rc.h"
#include "sql/expr/tuple.h"

class Record;
class TupleCellSpec;

class PhysicalOperator
{
public:
  PhysicalOperator()
  {}

  virtual ~PhysicalOperator();

  virtual RC open() = 0;
  virtual RC next() = 0;
  virtual RC close() = 0;

  virtual Tuple * current_tuple() = 0;

  void add_child(std::unique_ptr<PhysicalOperator> oper) {
    children_.emplace_back(std::move(oper));
  }

protected:
  std::vector<std::unique_ptr<PhysicalOperator>> children_;
};
