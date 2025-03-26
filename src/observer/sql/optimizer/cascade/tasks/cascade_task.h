/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#pragma once

#include "common/lang/vector.h"

class Rule;
class GroupExpr;
class RuleSet;
class OptimizerContext;
class Memo;
class RuleWithPromise;

enum class CascadeTaskType
{
  OPTIMIZE_GROUP,
  OPTIMIZE_EXPR,
  EXPLORE_GROUP,
  APPLY_RULE,
  OPTIMIZE_INPUTS
};

class CascadeTask
{
public:
  CascadeTask(OptimizerContext *context, CascadeTaskType type) : type_(type), context_(context) {}

  virtual void perform() = 0;

  Memo &get_memo() const;

  RuleSet &get_rule_set() const;

  void push_task(CascadeTask *task);

  virtual ~CascadeTask() = default;

protected:
  CascadeTaskType   type_;
  OptimizerContext *context_;
};