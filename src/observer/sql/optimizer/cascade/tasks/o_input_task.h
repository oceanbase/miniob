/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#pragma once

#include "sql/optimizer/cascade/tasks/cascade_task.h"
#include "sql/optimizer/cascade/property_set.h"

/**
 * OptimizeInputs
 */
class OptimizeInputs : public CascadeTask
{
public:
  OptimizeInputs(GroupExpr *group_expr, OptimizerContext *context)
      : CascadeTask(context, CascadeTaskType::OPTIMIZE_INPUTS), group_expr_(group_expr)
  {}

  explicit OptimizeInputs(OptimizeInputs *task)
      : CascadeTask(task->context_, CascadeTaskType::OPTIMIZE_INPUTS),
        group_expr_(task->group_expr_),
        cur_total_cost_(task->cur_total_cost_),
        cur_child_idx_(task->cur_child_idx_)
  {}

  void perform() override;

  ~OptimizeInputs() override {}

private:
  GroupExpr *group_expr_;

  double cur_total_cost_;

  /**
   * input currently being or about to be optimized
   */
  int cur_child_idx_ = -1;

  /**
   * keep track of the previous optimized input idx
   */
  int prev_child_idx_ = -1;
};