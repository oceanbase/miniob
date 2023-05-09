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
// Created by Wangyunlai on 2022/12/13.
//

#include "sql/optimizer/expression_rewriter.h"
#include "sql/optimizer/comparison_simplification_rule.h"
#include "sql/optimizer/conjunction_simplification_rule.h"
#include "common/log/log.h"

ExpressionRewriter::ExpressionRewriter()
{
  expr_rewrite_rules_.emplace_back(new ComparisonSimplificationRule);
  expr_rewrite_rules_.emplace_back(new ConjunctionSimplificationRule);
}

RC ExpressionRewriter::rewrite(std::unique_ptr<LogicalOperator> &oper, bool &change_made)
{
  RC rc = RC::SUCCESS;

  bool sub_change_made = false;
  std::vector<std::unique_ptr<Expression>> &expressions = oper->expressions();
  for (std::unique_ptr<Expression> &expr : expressions) {
    rc = rewrite_expression(expr, sub_change_made);
    if (rc != RC::SUCCESS) {
      break;
    }

    if (sub_change_made && !change_made) {
      change_made = true;
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

RC ExpressionRewriter::rewrite_expression(std::unique_ptr<Expression> &expr, bool &change_made)
{
  RC rc = RC::SUCCESS;

  change_made = false;
  for (std::unique_ptr<ExpressionRewriteRule> &rule : expr_rewrite_rules_) {
    bool sub_change_made = false;
    rc = rule->rewrite(expr, sub_change_made);
    if (sub_change_made && !change_made) {
      change_made = true;
    }
    if (rc != RC::SUCCESS) {
      break;
    }
  }

  if (change_made || rc != RC::SUCCESS) {
    return rc;
  }

  switch (expr->type()) {
    case ExprType::FIELD:
    case ExprType::VALUE: {
      // do nothing
    } break;

    case ExprType::CAST: {
      std::unique_ptr<Expression> &child_expr = (static_cast<CastExpr *>(expr.get()))->child();
      rc = rewrite_expression(child_expr, change_made);
    } break;

    case ExprType::COMPARISON: {
      auto comparison_expr = static_cast<ComparisonExpr *>(expr.get());
      std::unique_ptr<Expression> &left_expr = comparison_expr->left();
      std::unique_ptr<Expression> &right_expr = comparison_expr->right();

      bool left_change_made = false;
      rc = rewrite_expression(left_expr, left_change_made);
      if (rc != RC::SUCCESS) {
        return rc;
      }

      bool right_change_made = false;
      rc = rewrite_expression(right_expr, right_change_made);
      if (rc != RC::SUCCESS) {
        return rc;
      }

      if (left_change_made || right_change_made) {
        change_made = true;
      }
    } break;

    case ExprType::CONJUNCTION: {
      auto conjunction_expr = static_cast<ConjunctionExpr *>(expr.get());
      std::vector<std::unique_ptr<Expression>> &children = conjunction_expr->children();
      for (std::unique_ptr<Expression> &child_expr : children) {
        bool sub_change_made = false;
        rc = rewrite_expression(child_expr, sub_change_made);
        if (rc != RC::SUCCESS) {

          LOG_WARN("failed to rewriter conjunction sub expression. rc=%s", strrc(rc));
          return rc;
        }

        if (sub_change_made && !change_made) {
          change_made = true;
        }
      }
    } break;

    default: {
      // do nothing
    } break;
  }
  return rc;
}
