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

#include "sql/optimizer/comparison_simplification_rule.h"
#include "sql/expr/expression.h"
#include "common/log/log.h"

RC ComparisonSimplificationRule::rewrite(std::unique_ptr<Expression> &expr, bool &change_made)
{
  RC rc = RC::SUCCESS;
  change_made = false;
  if (expr->type() == ExprType::COMPARISON) {
    ComparisonExpr *cmp_expr = static_cast<ComparisonExpr *>(expr.get());
    Value value;
    RC sub_rc = cmp_expr->try_get_value(value);
    if (sub_rc == RC::SUCCESS) {
      std::unique_ptr<Expression> new_expr(new ValueExpr(value));
      expr.swap(new_expr);
      change_made = true;
      LOG_TRACE("comparison expression is simplified");
    }
  }
  return rc;
}
