/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "sql/optimizer/cascade/cost_model.h"
#include "sql/optimizer/cascade/memo.h"
#include "catalog/catalog.h"
#include "sql/optimizer/cascade/group_expr.h"

double CostModel::calculate_cost(Memo *memo,
                               GroupExpr *gexpr)
{
  auto op = gexpr->get_op();
  auto log_prop = memo->get_group_by_id(gexpr->get_group_id())->get_logical_prop();
  int arity = gexpr->get_children_groups_size();
  vector<LogicalProperty*> child_log_props;
  for (int i = 0; i < arity; ++i) {
    auto child_group_id = gexpr->get_child_group_id(i);
    auto child_gexpr = memo->get_group_by_id(child_group_id);
    child_log_props.push_back(child_gexpr->get_logical_prop());
  }
  return op->calculate_cost(log_prop, child_log_props, this);

}