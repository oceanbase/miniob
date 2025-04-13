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
#include "common/lang/memory.h"
#include "catalog/catalog.h"
#include "sql/optimizer/cascade/cost_model.h"
#include "sql/optimizer/cascade/tasks/cascade_task.h"
#include "sql/optimizer/cascade/pending_tasks.h"
#include "sql/operator/operator_node.h"
#include "sql/optimizer/cascade/property_set.h"

class Memo;
class RuleSet;
/**
 * OptimizerContext is a class containing pointers to various objects
 * that are required during the entire query optimization process.
 */
class OptimizerContext
{
public:
  OptimizerContext();

  ~OptimizerContext();

  Memo &get_memo();

  RuleSet &get_rule_set();

  void push_task(CascadeTask *task) { task_pool_->push(task); }

  CostModel *get_cost_model() { return &cost_model_; }

  void set_task_pool(PendingTasks *pending_tasks)
  {
    if (task_pool_ != nullptr) {
      delete task_pool_;
    }
    task_pool_ = pending_tasks;
  }

  void record_operator_node_in_memo(unique_ptr<OperatorNode> &&node);

  GroupExpr *make_group_expression(OperatorNode *node);

  bool record_node_into_group(OperatorNode *node, GroupExpr **gexpr) { return record_node_into_group(node, gexpr, -1); }

  bool record_node_into_group(OperatorNode *node, GroupExpr **gexpr, int target_group);

  double get_cost_upper_bound() const { return cost_upper_bound_; }

private:
  Memo         *memo_;
  RuleSet      *rule_set_;
  CostModel     cost_model_;
  PendingTasks *task_pool_;
  double        cost_upper_bound_;
};