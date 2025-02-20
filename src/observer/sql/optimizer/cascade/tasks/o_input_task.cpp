/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "sql/optimizer/cascade/tasks/o_input_task.h"
#include "sql/optimizer/cascade/optimizer_context.h"
#include "sql/optimizer/cascade/tasks/o_group_task.h"
#include "sql/optimizer/cascade/group_expr.h"
#include "sql/optimizer/cascade/memo.h"
#include "common/log/log.h"

void OptimizeInputs::perform()
{
  LOG_TRACE("OptimizeInputs::perform()");
  if (cur_child_idx_ == -1) {
    cur_total_cost_ = 0;

    cur_child_idx_ = 0;
    // only calculate once for current group expr
    cur_total_cost_ += context_->get_cost_model()->calculate_cost(
          &context_->get_memo(), group_expr_);
  }
  for (; cur_child_idx_ < static_cast<int>(group_expr_->get_children_groups_size()); cur_child_idx_++) {
    auto child_group =
        context_->get_memo().get_group_by_id(group_expr_->get_child_group_id(cur_child_idx_));

    // check whether the child group is already optimized
    auto child_best_expr = child_group->get_winner();
    if (child_best_expr != nullptr) {
      cur_total_cost_ += child_best_expr->get_cost();
      LOG_INFO("cur_total_cost_ = %f", cur_total_cost_);
      if (cur_total_cost_ > context_->get_cost_upper_bound()) break;
    } else if (prev_child_idx_ != cur_child_idx_) {  // we haven't optimized child group
      prev_child_idx_ = cur_child_idx_;
      push_task(new OptimizeInputs(this));
      push_task(new OptimizeGroup(child_group, context_));
      return;
    }
  }

    if (cur_child_idx_ == static_cast<int>(group_expr_->get_children_groups_size())) {
      group_expr_->set_local_cost(cur_total_cost_);

      auto cur_group = get_memo().get_group_by_id(group_expr_->get_group_id());
      cur_group->set_expr_cost(group_expr_, cur_total_cost_);
    }
}