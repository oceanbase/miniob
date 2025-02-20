/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "sql/optimizer/cascade/tasks/e_group_task.h"
#include "sql/optimizer/cascade/tasks/o_expr_task.h"
#include "common/log/log.h"

void ExploreGroup::perform()
{
  LOG_TRACE("ExploreGroup::perform() ");
  if (group_->has_explored()) {
    return;
  }

  for (auto &logical_expr : group_->get_logical_expressions()) {
    push_task(new OptimizeExpression(logical_expr, context_));
  }

  group_->set_explored();
}