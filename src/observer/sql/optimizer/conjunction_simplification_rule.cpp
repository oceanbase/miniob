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
// Created by Wangyunlai on 2022/12/26.
//

#include "common/log/log.h"
#include "sql/optimizer/conjunction_simplification_rule.h"

RC ConjunctionSimplificationRule::rewrite(std::unique_ptr<Expression> &expr, bool &change_made)
{
  RC rc = RC::SUCCESS;
  if (expr->type() != ExprType::CONJUNCTION) {
    return rc;
  }

  change_made = false;
  auto conjunction_expr = static_cast<ConjunctionExpr *>(expr.get());
  std::vector<std::unique_ptr<Expression>> &child_exprs = conjunction_expr->children();
  if (child_exprs.size() == 1) {
    LOG_TRACE("conjunction expression has only 1 child");
    std::unique_ptr<Expression> child_expr = std::move(child_exprs.front());
    child_exprs.clear();
    expr = std::move(child_expr);

    change_made = true;
  }

  // TODO check whether have child expressions always true or false
  return rc;
}
