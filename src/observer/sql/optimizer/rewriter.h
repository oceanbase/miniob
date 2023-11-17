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
#include <memory>
#include <vector>

class LogicalOperator;

/**
 * @defgroup Rewriter
 * @brief 根据规则对逻辑计划进行重写
 */

/**
 * @brief 根据一些规则对逻辑计划进行重写
 * @ingroup Rewriter
 * @details 当前仅实现了一两个非常简单的规则。
 * 重写包括对逻辑计划和计划中包含的表达式。
 */
class Rewriter
{
public:
  Rewriter();
  virtual ~Rewriter() = default;

  /**
   * @brief 对逻辑计划进行重写
   * @details 如果重写发生，change_made为true，否则为false。
   * 通常情况下如果改写发生改变，就会继续重写，直到没有改变为止。
   * @param oper 逻辑计划
   * @param change_made 当前是否有重写发生
   */
  RC rewrite(std::unique_ptr<LogicalOperator> &oper, bool &change_made);

private:
  std::vector<std::unique_ptr<RewriteRule>> rewrite_rules_;
};
