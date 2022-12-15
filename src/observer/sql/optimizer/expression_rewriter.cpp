/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Wangyunlai on 2022/12/13.
//

#include "sql/optimizer/expression_rewriter.h"
#include "sql/optimizer/comparison_simplification_rule.h"

ExpressionRewriter::ExpressionRewriter()
{
  expr_rewrite_rules_.emplace_back(new ComparisonSimplificationRule);
}

RC ExpressionRewriter::rewrite(std::unique_ptr<LogicalOperator> &oper, bool &change_made)
{
  RC rc = RC::SUCCESS;
  
  change_made = false;
  std::vector<std::unique_ptr<Expression>> &expressions = oper->expressions();
  for (std::unique_ptr<Expression> &expr : expressions) {
    for (std::unique_ptr<ExpressionRewriteRule> & rule: expr_rewrite_rules_) {
      bool sub_change_made = false;
      rc = rule->rewrite(expr, sub_change_made);
      if (sub_change_made && !change_made) {
        change_made = true;
      }
      if (rc != RC::SUCCESS) {
        break;
      }
    }

    if (rc != RC::SUCCESS) {
      break;
    }
  }

  if (rc != RC::SUCCESS) {
    return rc;
  }

  std::vector<std::unique_ptr<LogicalOperator>> &child_opers = oper->children();
  for (std::unique_ptr<LogicalOperator> &child_oper : child_opers) {
    bool sub_change_made = false;
    rc = rewrite(child_oper, sub_change_made);
    if (sub_change_made && !change_made) {
      change_made = true;
    }
    if (rc != RC::SUCCESS) {
      break;
    }
  }
  return rc;
}
