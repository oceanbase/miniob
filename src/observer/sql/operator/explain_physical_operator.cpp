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

#include "sql/operator/explain_physical_operator.h"
#include "sql/optimizer/optimizer_utils.h"
#include "common/log/log.h"

using namespace std;

RC ExplainPhysicalOperator::open(Trx *)
{
  ASSERT(children_.size() == 1, "explain must has 1 child");
  return RC::SUCCESS;
}

RC ExplainPhysicalOperator::close() { return RC::SUCCESS; }

void ExplainPhysicalOperator::generate_physical_plan()
{
  ASSERT(children_.size() == 1, "explain must has 1 child");
  physical_plan_ = OptimizerUtils::dump_physical_plan(children_.front());
}

RC ExplainPhysicalOperator::next()
{
  if (!physical_plan_.empty()) {
    return RC::RECORD_EOF;
  }
  generate_physical_plan();

  vector<Value> cells;
  Value         cell(physical_plan_.c_str());
  cells.emplace_back(cell);
  tuple_.set_cells(cells);
  return RC::SUCCESS;
}

RC ExplainPhysicalOperator::next(Chunk &chunk)
{
  if (!physical_plan_.empty()) {
    return RC::RECORD_EOF;
  }
  generate_physical_plan();

  Value         cell(physical_plan_.c_str());
  auto column = make_unique<Column>();
  column->init(cell, chunk.rows());
  chunk.add_column(std::move(column), 0);
  return RC::SUCCESS;
}

Tuple *ExplainPhysicalOperator::current_tuple() { return &tuple_; }

