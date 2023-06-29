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

#include "common/rc.h"
#include "sql/optimizer/rewrite_rule.h"

class LogicalOperator;

/**
 * @brief 简单比较的重写规则
 * @ingroup Rewriter
 * @details 如果有简单的比较运算，比如比较的两边都是常量，那我们就可以在运行执行计划之前就知道结果，
 * 进而直接将表达式改成结果，这样就可以减少运行时的计算量。
 */
class ComparisonSimplificationRule : public ExpressionRewriteRule 
{
public:
  ComparisonSimplificationRule() = default;
  virtual ~ComparisonSimplificationRule() = default;

  RC rewrite(std::unique_ptr<Expression> &expr, bool &change_made) override;

private:
};
