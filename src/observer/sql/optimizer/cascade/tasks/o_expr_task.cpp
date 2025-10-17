/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "sql/optimizer/cascade/tasks/o_expr_task.h"
#include "sql/optimizer/cascade/tasks/apply_rule_task.h"
#include "sql/optimizer/cascade/tasks/e_group_task.h"
#include "sql/optimizer/cascade/group_expr.h"
#include "sql/optimizer/cascade/memo.h"
#include "common/log/log.h"
#include <algorithm>

void OptimizeExpression::perform()
{
  std::vector<RuleWithPromise> valid_rules;

  // Construct valid transformation rules from rule set
  // TODO: add transformation rules
  auto phys_rules = get_rule_set().get_rules_by_name(RuleSetName::PHYSICAL_IMPLEMENTATION);
  for (auto &rule : phys_rules) {
    // check if we can apply the rule
    bool already_explored = group_expr_->rule_explored(rule);
    if (already_explored) {
      continue;
    }
    bool root_pattern_mismatch = group_expr_->get_op()->get_op_type() != rule->get_match_pattern()->type();

    bool child_pattern_mismatch =
        group_expr_->get_children_groups_size() != rule->get_match_pattern()->get_child_patterns_size();

    if (root_pattern_mismatch || child_pattern_mismatch) {
      continue;
    }

    auto promise = rule->promise(group_expr_);
    valid_rules.emplace_back(rule, promise);
  }

  std::sort(valid_rules.begin(), valid_rules.end());
  LOG_TRACE("OptimizeExpression::perform() op {%d}, valid rules : {%d}",
                      static_cast<int>(group_expr_->get_op()->get_op_type()), valid_rules.size());
  // apply the rule
  for (auto &r : valid_rules) {
    push_task(new ApplyRule(group_expr_, r.get_rule(), context_));
    int child_group_idx = 0;
    for (auto &child_pattern : r.get_rule()->get_match_pattern()->children()) {
      if (child_pattern->get_child_patterns_size() > 0) {
        Group *group = get_memo().get_group_by_id(group_expr_->get_child_group_ids()[child_group_idx]);
        push_task(new ExploreGroup(group, context_));
      }

      child_group_idx++;
    }
  }
}
