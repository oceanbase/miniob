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
// Created by WangYunlai on 2022/6/27.
//

#pragma once

#include "sql/operator/operator.h"

class FilterStmt;

/**
 * PredicateOperator 用于单个表中的记录过滤
 * 如果是多个表数据过滤，比如join条件的过滤，需要设计新的predicate或者扩展:w
 */
class PredicateOperator : public Operator
{
public:
  PredicateOperator(FilterStmt *filter_stmt)
    : filter_stmt_(filter_stmt)
  {}

  virtual ~PredicateOperator() = default;

  RC open() override;
  RC next() override;
  RC close() override;

  Tuple * current_tuple() override;
  //int tuple_cell_num() const override;
  //RC tuple_cell_spec_at(int index, TupleCellSpec &spec) const override;
private:
  bool do_predicate(RowTuple &tuple);
private:
  FilterStmt *filter_stmt_ = nullptr;
};
