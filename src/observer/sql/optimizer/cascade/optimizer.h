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

#include "sql/optimizer/cascade/pending_tasks.h"
#include "sql/optimizer/cascade/optimizer_context.h"
#include "sql/operator/physical_operator.h"
#include "sql/optimizer/cascade/property_set.h"

/**
 * @brief cascade optimizer
 * TODO: currently, Optimizer is used for CBO optimization. need to unify the optimizer in miniob
 */
class Optimizer
{
public:
  Optimizer() : context_(std::make_unique<OptimizerContext>()) {}

  std::unique_ptr<PhysicalOperator> optimize(OperatorNode *op_tree);

  std::unique_ptr<PhysicalOperator> choose_best_plan(int root_id);

private:
  void optimize_loop(int root_group_id);

  void execute_task_stack(PendingTasks *task_stack, int root_group_id, OptimizerContext *root_context);

  CostModel                         cost_model_;
  std::unique_ptr<OptimizerContext> context_;
};