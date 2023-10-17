/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Wangyunlai on 2022/12/29.
//

#include "sql/optimizer/rewriter.h"
#include "sql/optimizer/expression_rewriter.h"
#include "sql/optimizer/predicate_rewrite.h"
#include "sql/optimizer/predicate_pushdown_rewriter.h"
#include "sql/operator/logical_operator.h"

Rewriter::Rewriter()
{
  rewrite_rules_.emplace_back(new ExpressionRewriter);
  rewrite_rules_.emplace_back(new PredicateRewriteRule);
  rewrite_rules_.emplace_back(new PredicatePushdownRewriter);
}

RC Rewriter::rewrite(std::unique_ptr<LogicalOperator> &oper, bool &change_made)
{
  RC rc = RC::SUCCESS;

  change_made = false;
  for (std::unique_ptr<RewriteRule> &rule : rewrite_rules_) {
    bool sub_change_made = false;
    rc = rule->rewrite(oper, sub_change_made);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to rewrite logical operator. rc=%s", strrc(rc));
      return rc;
    }

    if (sub_change_made && !change_made) {
      change_made = true;
    }
  }

  if (rc != RC::SUCCESS) {
    return rc;
  }

  std::vector<std::unique_ptr<LogicalOperator>> &child_opers = oper->children();
  for (auto &child_oper : child_opers) {
    bool sub_change_made = false;
    rc = this->rewrite(child_oper, sub_change_made);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to rewrite child oper. rc=%s", strrc(rc));
      return rc;
    }

    if (sub_change_made && !change_made) {
      change_made = true;
    }
  }
  return rc;
}
