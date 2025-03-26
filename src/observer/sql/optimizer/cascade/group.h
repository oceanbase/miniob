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

#include "common/lang/limits.h"
#include "common/lang/vector.h"
#include "common/lang/unordered_set.h"
#include "common/lang/memory.h"
#include "common/log/log.h"
#include "common/lang/unordered_map.h"
#include "sql/optimizer/cascade/property_set.h"

class GroupExpr;
class Memo;
/**
 * @class Group
 *
 * @brief A class representing a group within cascade optimizer.
 *
 * This class manages a group of expressions and their associated costs,
 * and keeps track of whether the group has been explored.
 */
class Group
{
public:
  /**
   * @brief Constructor for the Group class. A Group tracks both logical and
   * physical M_EXPRs.
   *
   * @param id The unique identifier for the group.
   */
  Group(int id, GroupExpr *expr, Memo *memo);

  ~Group();

  /**
   * @brief Adds an expression to the group.
   *
   * @param expr The expression to add.
   */
  void add_expr(GroupExpr *expr);

  /**
   * @brief Sets the cost of a given expression in the group.
   *
   * @param expr The expression for which to set the cost.
   * @param cost The cost associated with the expression.
   * @return True if the cost was successfully set, false if the expression is not setted.
   */
  bool set_expr_cost(GroupExpr *expr, double cost);

  /**
   * @return The expression with the lowest cost.
   */
  GroupExpr *get_winner();

  /**
   * @brief Gets the logical expressions in the group.
   */
  const std::vector<GroupExpr *> &get_logical_expressions() const { return logical_expressions_; }

  /**
   * @brief Gets the physical expressions in the group.
   */
  const std::vector<GroupExpr *> &get_physical_expressions() const { return physical_expressions_; }

  /**
   * @brief Gets the cost lower bound.
   *
   * @note This function is a work in progress (WIP).
   */
  double get_cost_lb() { return -1; }

  /**
   * @brief Marks the group as explored.
   */
  void set_explored() { has_explored_ = true; }

  /**
   * @brief Checks if the group has been explored.
   */
  bool has_explored() { return has_explored_; }

  int get_id() const { return id_; }

  GroupExpr *get_logical_expression();

  LogicalProperty *get_logical_prop() { return logical_prop_.get(); }

  ///< dump the group info, for debug
  void dump() const;

private:
  int id_;

  std::tuple<double, GroupExpr *> winner_;

  bool has_explored_;

  std::vector<GroupExpr *> logical_expressions_;

  std::vector<GroupExpr *> physical_expressions_;

  unique_ptr<LogicalProperty> logical_prop_ = nullptr;
};