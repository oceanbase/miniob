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
// Created by Wangyunlai on 2022/12/26.
//

#pragma once

#include "sql/optimizer/rewrite_rule.h"

class LogicalOperator;

/**
 * @brief 简化多个表达式联结的运算
 * @ingroup Rewriter
 * @details 比如只有一个表达式，或者表达式可以直接出来
 */
class ConjunctionSimplificationRule : public ExpressionRewriteRule 
{
public:
  ConjunctionSimplificationRule() = default;
  virtual ~ConjunctionSimplificationRule() = default;

  RC rewrite(std::unique_ptr<Expression> &expr, bool &change_made) override;

private:
};
