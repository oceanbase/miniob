/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "sql/optimizer/cascade/group.h"
#include "sql/optimizer/cascade/group_expr.h"
#include "sql/optimizer/cascade/memo.h"

Group::Group(int id, GroupExpr* expr, Memo *memo)
      : id_(id), winner_(std::make_tuple(numeric_limits<double>::max(), nullptr)), has_explored_(false)
{
  int arity = expr->get_children_groups_size();
	vector<LogicalProperty*> input_prop;
	if(arity == 0) {
    logical_prop_ = (expr->get_op())->find_log_prop(input_prop);
	} else {
		for(int i=0; i<arity; i++)
		{	
			auto group = memo->get_group_by_id(expr->get_child_group_id(i));
			input_prop.push_back(group->get_logical_prop());
		}
		
		logical_prop_ = (expr->get_op())->find_log_prop(input_prop);
  }
}
Group::~Group() {
  for (auto expr : logical_expressions_) {
    delete expr;
  }
  for (auto expr : physical_expressions_) {
    delete expr;
  }
}

void Group::add_expr(GroupExpr *expr)
{
  expr->set_group_id(id_);
  if (expr->get_op()->is_physical()) {
    physical_expressions_.push_back(expr);
  } else {
    logical_expressions_.push_back(expr);
  }
}

bool Group::set_expr_cost(GroupExpr *expr, double cost) {
  

  if (std::get<0>(winner_) > cost) {
    // this is lower cost
    winner_ = std::make_tuple(cost, expr);
    return true;
  }
  return false;
}

GroupExpr *Group::get_winner() {
  return std::get<1>(winner_);
}

GroupExpr *Group::get_logical_expression() {
  ASSERT(logical_expressions_.size() == 1, "There should exist only 1 logical expression");
  ASSERT(physical_expressions_.empty(), "No physical expressions should be present");
  return logical_expressions_[0];
}

void Group::dump() const
{
  LOG_TRACE("Group %d has %lu logical expressions and %lu physical expressions", 
           id_, logical_expressions_.size(), physical_expressions_.size(), physical_expressions_.size());
}