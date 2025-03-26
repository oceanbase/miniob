/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "sql/optimizer/cascade/optimizer.h"
#include "sql/optimizer/cascade/tasks/o_group_task.h"
#include "sql/optimizer/cascade/memo.h"

std::unique_ptr<PhysicalOperator> Optimizer::optimize(OperatorNode* op_tree)
{
  // Generate initial operator tree from query tree
  GroupExpr *gexpr = nullptr;
  bool insert = context_->record_node_into_group(op_tree, &gexpr);
  ASSERT(insert && gexpr, "Logical expression tree should insert");
  context_->get_memo().dump();

  int root_id = gexpr->get_group_id();

  optimize_loop(root_id);
  LOG_TRACE("after optimize, memo dump:");
  context_->get_memo().dump();
  return choose_best_plan(root_id);
}

std::unique_ptr<PhysicalOperator> Optimizer::choose_best_plan(int root_group_id)
{
  auto &memo = context_->get_memo();
  Group *root_group = memo.get_group_by_id(root_group_id);
  ASSERT(root_group != nullptr, "Root group should not be null");

  // Choose the best physical plan
  auto winner = root_group->get_winner();
  if (winner == nullptr) {
    LOG_WARN("No winner found in group %d", root_group_id);
    return nullptr;
  }
  auto winner_contents = winner->get_op();
  context_->get_memo().release_operator(winner_contents);
  PhysicalOperator* winner_phys = dynamic_cast<PhysicalOperator*>(winner_contents);
  LOG_TRACE("winner: %d", winner_phys->type());
  for (const auto& child : winner->get_child_group_ids()) {
    winner_phys->add_child(choose_best_plan(child));
  }

  return std::unique_ptr<PhysicalOperator>(winner_phys);
}

void Optimizer::optimize_loop(int root_group_id)
{
  auto task_stack = new PendingTasks();
  context_->set_task_pool(task_stack);

  Memo &memo = context_->get_memo();
  task_stack->push(new OptimizeGroup(memo.get_group_by_id(root_group_id), context_.get()));

  execute_task_stack(task_stack, root_group_id, context_.get());
}

void Optimizer::execute_task_stack(PendingTasks *task_stack, int root_group_id, OptimizerContext *root_context)
{
  while (!task_stack->empty()) {
    auto task = task_stack->pop();
    task->perform();
    delete task;
  }
}
