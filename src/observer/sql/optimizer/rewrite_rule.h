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

#pragma once

#include "common/sys/rc.h"
#include "common/lang/memory.h"

class LogicalOperator;
class Expression;

/**
 * @brief 逻辑计划的重写规则
 * @ingroup Rewriter
 * TODO: 重构下当前的查询改写规则，放到 cascade optimizer 中。
 */
class RewriteRule
{
public:
  virtual ~RewriteRule() = default;

  virtual RC rewrite(unique_ptr<LogicalOperator> &oper, bool &change_made) = 0;
};

/**
 * @brief 表达式的重写规则
 * @ingroup Rewriter
 */
class ExpressionRewriteRule
{
public:
  virtual ~ExpressionRewriteRule() = default;

  virtual RC rewrite(unique_ptr<Expression> &expr, bool &change_made) = 0;
};
