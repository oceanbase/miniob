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
// Created by Wangyunlai on 2022/12/30.
//

#pragma once

#include "sql/optimizer/rewrite_rule.h"
#include <vector>

/**
 * @brief 将一些谓词表达式下推到表数据扫描中
 * @ingroup Rewriter
 * @details 这样可以提前过滤一些数据
 */
class PredicatePushdownRewriter : public RewriteRule
{
public:
  PredicatePushdownRewriter()          = default;
  virtual ~PredicatePushdownRewriter() = default;

  RC rewrite(std::unique_ptr<LogicalOperator> &oper, bool &change_made) override;

private:
  RC get_exprs_can_pushdown(
      std::unique_ptr<Expression> &expr, std::vector<std::unique_ptr<Expression>> &pushdown_exprs);
};
