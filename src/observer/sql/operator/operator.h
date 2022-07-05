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
// Created by WangYunlai on 2021/6/7.
//

#pragma once

#include <vector>
#include "rc.h"
#include "sql/expr/tuple.h"

class Record;
class TupleCellSpec;

class Operator
{
public:
  Operator()
  {}

  virtual ~Operator() = default;

  virtual RC open() = 0;
  virtual RC next() = 0;
  virtual RC close() = 0;

  virtual Tuple * current_tuple() = 0;
  //virtual int tuple_cell_num() const = 0;
  //virtual RC  tuple_cell_spec_at(int index, TupleCellSpec *&spec) const = 0;

  void add_child(Operator *oper) {
    children_.push_back(oper);
  }


protected:
  std::vector<Operator *> children_;
};
