/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "sql/optimizer/cascade/tasks/o_group_task.h"
#include "sql/optimizer/cascade/tasks/o_input_task.h"
#include "sql/optimizer/cascade/tasks/o_expr_task.h"
#include "sql/optimizer/cascade/optimizer_context.h"

void OptimizeGroup::perform()
{
  LOG_TRACE("OptimizeGroup::perform() group %d", group_->get_id());
  // TODO: currently the cost upper bound is not used
  if (group_->get_cost_lb() > context_->get_cost_upper_bound()) {
    return;
  }
  if (group_->get_winner() != nullptr) {
    return;
  }

  if (!group_->has_explored()) {
    for (auto &logical_expr : group_->get_logical_expressions()) {
      push_task(new OptimizeExpression(logical_expr, context_));
    }
  }

  for (auto &physical_expr : group_->get_physical_expressions()) {
    push_task(new OptimizeInputs(physical_expr, context_));
  }

  group_->set_explored();
}