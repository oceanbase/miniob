/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "sql/optimizer/cascade/tasks/apply_rule_task.h"
#include "sql/optimizer/cascade/tasks/o_input_task.h"
#include "sql/optimizer/cascade/tasks/o_expr_task.h"
#include "sql/optimizer/cascade/group_expr.h"
#include "sql/optimizer/cascade/rules.h"
#include "common/log/log.h"

void ApplyRule::perform()
{
  LOG_TRACE("ApplyRule::perform() for rule: {%d}", rule_->get_rule_idx());
  if (group_expr_->rule_explored(rule_)) {
    return;
  }
  // TODO: expr binding, currently group_expr_->get_op() is enough
  OperatorNode* before = group_expr_->get_op();

  // TODO: check condition

  std::vector<unique_ptr<OperatorNode>> after;
  rule_->transform(before, &after, context_);
  for (const auto &new_expr : after) {
    GroupExpr *new_gexpr = nullptr;
    auto g_id = group_expr_->get_group_id();
    if (context_->record_node_into_group(new_expr.get(), &new_gexpr, g_id)) {
      if (new_gexpr->get_op()->is_logical()) {
        // further optimize new expr
        push_task(new OptimizeExpression(new_gexpr, context_));
      } else {
        // calculate the cost of the new physical expr
        push_task(new OptimizeInputs(new_gexpr, context_));
      }
    } else {
      LOG_INFO("record_operator_node_into_group not insert new expr");
      new_gexpr->dump();
    }
  }
  
  // TODO: FIXME, better way for record memory allocation
  for (size_t i = 0; i < after.size(); i++) {
    context_->record_operator_node_in_memo(std::move(after[i]));
  }

  group_expr_->set_rule_explored(rule_);
}