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

#pragma once

#include "sql/optimizer/rewrite_rule.h"

/**
 * @brief 谓词重写规则
 * @ingroup Rewriter
 * @details 有些谓词可以在真正运行之前就知道结果，那么就可以提前运算出来，比如1=1,1=0。
 */
class PredicateRewriteRule : public RewriteRule
{
public:
  PredicateRewriteRule()          = default;
  virtual ~PredicateRewriteRule() = default;

  RC rewrite(std::unique_ptr<LogicalOperator> &oper, bool &change_made) override;
};
