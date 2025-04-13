/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "sql/optimizer/cascade/memo.h"

GroupExpr *Memo::insert_expression(GroupExpr *gexpr, int target_group)
{
  gexpr->set_group_id(target_group);
  auto it = group_expressions_.find(gexpr);
  if (it != group_expressions_.end()) {
    ASSERT(*gexpr == *(*it), "GroupExpression should be equal");
    delete gexpr;
    return *it;
  }

  group_expressions_.insert(gexpr);

  // New expression, so try to insert into an existing group or
  // create a new group if none specified
  int group_id;
  if (target_group == -1) {
    group_id = add_new_group(gexpr);
  } else {
    group_id = target_group;
  }

  Group *group = get_group_by_id(group_id);
  group->add_expr(gexpr);
  return gexpr;
}

int Memo::add_new_group(GroupExpr *gexpr)
{
  auto new_group_id = int(groups_.size());

  groups_.push_back(unique_ptr<Group>(new Group(new_group_id, gexpr, this)));
  return new_group_id;
}

void Memo::dump() const
{
  LOG_TRACE("Memo has %lu groups", groups_.size());
  for (const auto &group : groups_) {
    group->dump();
  }
}