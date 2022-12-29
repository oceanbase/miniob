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
// Created by WangYunlai on 2022/12/27.
//

#include <sstream>
#include "sql/operator/explain_physical_operator.h"
#include "common/log/log.h"

using namespace std;

RC ExplainPhysicalOperator::open()
{
  ASSERT(children_.size() == 1, "explain must has 1 child");
  return RC::SUCCESS;
}

RC ExplainPhysicalOperator::close()
{
  for (std::unique_ptr<PhysicalOperator> &child_oper : children_) {
    child_oper->close();
  }
  return RC::SUCCESS;
}

RC ExplainPhysicalOperator::next()
{
  if (!physical_plan_.empty()) {
    return RC::RECORD_EOF;
  }
  
  stringstream ss;
  ss << "OPERATOR(NAME)\n";
  
  int level = 0;
  for (unique_ptr<PhysicalOperator> &child_oper : children_) {
    to_string(ss, child_oper.get(), level);
  }

  physical_plan_ = ss.str();

  std::vector<TupleCell> cells;
  TupleCell cell;
  cell.set_string(physical_plan_.c_str());
  cells.emplace_back(cell);
  tuple_.set_cells(cells);
  return RC::SUCCESS;
}

Tuple *ExplainPhysicalOperator::current_tuple()
{
  return &tuple_;
}

void ExplainPhysicalOperator::to_string(std::ostream &os, PhysicalOperator *oper, int level)
{
  for (int i = 0; i < level; i++) {
    os << ' ';
  }

  os << oper->name();
  std::string param = oper->param();
  if (!param.empty()) {
    os << "(" << param << ")";
  }
  os << '\n';

  std::vector<std::unique_ptr<PhysicalOperator>> &children = oper->children();
  for (std::unique_ptr<PhysicalOperator> &child_oper : children) {
    to_string(os, child_oper.get(), level + 1);
  }
}
