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

#include "common/lang/bitset.h"
#include "common/log/log.h"
#include "common/lang/unordered_map.h"
#include "sql/operator/operator_node.h"
#include "sql/optimizer/cascade/rules.h"
#include "sql/optimizer/cascade/property_set.h"

// TODO: rename to m_expr(in columbia)?
/* GroupExpr used to represent a particular logical or physical
 * operator expression.
 */
class GroupExpr
{
public:
  /**
   * @param contents optimizer node contents
   * @param child_groups Vector of children groups
   */
  GroupExpr(OperatorNode *contents, std::vector<int> &&child_groups)
      : group_id_(-1), contents_(contents), child_groups_(child_groups)
  {}

  ~GroupExpr() {}

  int get_group_id() const { return group_id_; }

  void set_group_id(int id) { group_id_ = id; }

  const vector<int> &get_child_group_ids() const { return child_groups_; }

  int get_child_group_id(int child_idx) const
  {
    ASSERT(child_idx >= 0 && static_cast<size_t>(child_idx) < child_groups_.size(),
                     "child_idx is out of bounds");
    return child_groups_[child_idx];
  }

  OperatorNode *get_op() { return contents_; }

  double get_cost() const { return lowest_cost_; }

  void set_local_cost(double cost)
  {
    if (cost < lowest_cost_) {
      lowest_cost_ = cost;
    }
  }

  // TODO
  uint64_t hash() const;

  bool operator==(const GroupExpr &r) const
  {
    return (*contents_ == *(r.contents_)) && (child_groups_ == r.child_groups_);
  }

  void set_rule_explored(Rule *rule) { rule_mask_.set(rule->get_rule_idx(), true); }

  bool rule_explored(Rule *rule) { return rule_mask_.test(rule->get_rule_idx()); }

  size_t get_children_groups_size() const { return child_groups_.size(); }

  void dump() const;

private:
  int group_id_{};

  OperatorNode *contents_{};

  std::vector<int> child_groups_;

  std::bitset<static_cast<uint32_t>(RuleType::NUM_RULES)> rule_mask_;

  double lowest_cost_ = std::numeric_limits<double>::max();
};