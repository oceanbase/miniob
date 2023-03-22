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

#pragma once

#include "rc.h"
#include "sql/optimizer/rewrite_rule.h"

class LogicalOperator;

class ComparisonSimplificationRule : public ExpressionRewriteRule {
public:
  ComparisonSimplificationRule() = default;
  virtual ~ComparisonSimplificationRule() = default;

  RC rewrite(std::unique_ptr<Expression> &expr, bool &change_made) override;

private:
};
