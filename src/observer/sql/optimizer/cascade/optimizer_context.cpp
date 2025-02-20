/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "sql/optimizer/cascade/optimizer_context.h"
#include "sql/optimizer/cascade/memo.h"
#include "sql/optimizer/cascade/rules.h"

OptimizerContext::OptimizerContext()
      : memo_(new Memo()), rule_set_(new RuleSet()), cost_model_(), task_pool_(nullptr),
        cost_upper_bound_(std::numeric_limits<double>::max()) {}

OptimizerContext::~OptimizerContext() {
    if (task_pool_ != nullptr) {
      delete task_pool_;
      task_pool_ = nullptr;
    }
    if (memo_ != nullptr) {
      delete memo_;
      memo_ = nullptr;
    }
    if (rule_set_ != nullptr) {
      delete rule_set_;
      rule_set_ = nullptr;
    }
  }

GroupExpr *OptimizerContext::make_group_expression(OperatorNode* node)
{
  std::vector<int> child_groups;
  for (auto &child : node->get_general_children()) {
    auto gexpr = make_group_expression(child);

    // Insert into the memo (this allows for duplicate detection)
    auto mexpr = memo_->insert_expression(gexpr);
    if (mexpr == nullptr) {
      // Delete if need to (see InsertExpression spec)
      child_groups.push_back(gexpr->get_group_id());
      delete gexpr;
    } else {
      child_groups.push_back(mexpr->get_group_id());
    }
  }
  return new GroupExpr(node, std::move(child_groups));
}

  bool OptimizerContext::record_node_into_group(OperatorNode* node, GroupExpr **gexpr,
                                    int target_group) {
    auto new_gexpr = make_group_expression(node);
    auto ptr = memo_->insert_expression(new_gexpr, target_group);
    ASSERT(ptr, "Root of expr should not fail insertion");

    (*gexpr) = ptr;
    return (ptr == new_gexpr);
  }

  Memo &OptimizerContext::get_memo() { return *memo_; }

  RuleSet &OptimizerContext::get_rule_set() { return *rule_set_; }

  void OptimizerContext::record_operator_node_in_memo(unique_ptr<OperatorNode>&& node)
  {
    memo_->record_operator(std::move(node));
  }