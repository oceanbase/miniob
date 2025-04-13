/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "sql/optimizer/cascade/tasks/cascade_task.h"
#include "sql/optimizer/cascade/optimizer_context.h"
#include "sql/optimizer/cascade/group_expr.h"
#include "sql/optimizer/cascade/rules.h"
#include "sql/optimizer/cascade/memo.h"

Memo &CascadeTask::get_memo() const {return context_->get_memo(); }

RuleSet &CascadeTask::get_rule_set() const { return context_->get_rule_set(); }

void CascadeTask::push_task(CascadeTask *task) { context_->push_task(task); }