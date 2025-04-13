/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "sql/optimizer/cascade/group_expr.h"

uint64_t GroupExpr::hash() const
{
  auto hash = contents_->hash();
  for (const auto &child : child_groups_) {
    hash ^= std::hash<int>()(child) + 0x9e3779b9 + (hash << 6) + (hash >> 2);;
  }
  return hash;
}

void GroupExpr::dump() const
{
  stringstream ss;
  for (const auto &child : child_groups_) {
    ss << child << " ";
  }
  LOG_TRACE("GroupExpr contents: %d child groups:  %s", static_cast<int>(contents_->get_op_type()), ss.str().c_str());
}