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

#include <stdint.h>
#include "common/lang/vector.h"
#include "common/lang/memory.h"
#include "sql/optimizer/cascade/property.h"
#include "sql/optimizer/cascade/cost_model.h"
/**
 * @brief Operator type(including logical and physical)
 */
enum class OpType
{
  UNDEFINED = 0,

  // Special wildcard
  LEAF,

  // Logical Operators
  LOGICALGET,
  LOGICALCALCULATE,
  LOGICALGROUPBY,
  LOGICALPROJECTION,
  LOGICALFILTER,
  LOGICALINNERJOIN,
  LOGICALINSERT,
  LOGICALDELETE,
  LOGICALUPDATE,
  LOGICALLIMIT,
  LOGICALANALYZE,
  LOGICALEXPLAIN,
  // Separation of logical and physical operators
  LOGICALPHYSICALDELIMITER,

  // Physical Operators
  EXPLAIN,
  CALCULATE,
  SEQSCAN,
  INDEXSCAN,
  ORDERBY,
  LIMIT,
  INNERINDEXJOIN,
  INNERNLJOIN,
  INNERHASHJOIN,
  PROJECTION,
  INSERT,
  DELETE,
  UPDATE,
  AGGREGATE,
  HASHGROUPBY,
  ANALYZE,
  FILTER,
  SCALARGROUPBY
};

// TODO: OperatorNode is the abstrace class of logical/physical operator
// in cascade there is EXPR to include OperatorNode and OperatorNode children
// so here remove genral_children.
class OperatorNode
{
public:
  virtual ~OperatorNode() = default;
  /**
   * TODO: add this function
   */
  // virtual std::string get_name() const = 0;

  /**
   * TODO: unify logical and physical OpType
   */
  virtual OpType get_op_type() const { return OpType::UNDEFINED; }

  /**
   * @return Whether node contents represent a physical operator / expression
   */
  virtual bool is_physical() const = 0;

  /**
   * @return Whether node represents a logical operator / expression
   */
  virtual bool is_logical() const = 0;

  /**
   * TODO: complete it if needed
   */
  virtual uint64_t hash() const { return std::hash<int>()(static_cast<int>(get_op_type())); }
  virtual bool     operator==(const OperatorNode &other) const
  {
    if (get_op_type() != other.get_op_type())
      return false;
    if (general_children_.size() != other.general_children_.size())
      return false;

    for (size_t idx = 0; idx < general_children_.size(); idx++) {
      auto &child       = general_children_[idx];
      auto &other_child = other.general_children_[idx];

      if (*child != *other_child)
        return false;
    }
    return true;
  }
  /**
   * @brief Generate the logical property of the operator node using the input logical properties.
   * @param log_props Input logical properties of the operator node.
   * @return Logical property of the operator node.
   */
  virtual unique_ptr<LogicalProperty> find_log_prop(const vector<LogicalProperty *> &log_props) { return nullptr; }

  /**
   * @brief Calculates the cost of a logical operation.
   *
   * This function is intended to be overridden in derived classes. It calculates
   * the cost associated with a specific logical property, taking into account the
   * provided child logical properties and a cost model.
   *
   * @param prop A pointer to the logical property for which the cost is being calculated.
   * @param child_log_props A vector containing pointers to child logical properties.
   * @param cm A pointer to the cost model used for calculating the cost.
   * @return The calculated cost as a double.
   */
  virtual double calculate_cost(LogicalProperty *prop, const vector<LogicalProperty *> &child_log_props, CostModel *cm)
  {
    return 0.0;
  }

  void add_general_child(OperatorNode *child) { general_children_.push_back(child); }

  vector<OperatorNode *> &get_general_children() { return general_children_; }

protected:
  // TODO: refactor
  // cascade optimizer 中使用，为了logical/physical operator 可以统一在 cascade 中迭代
  vector<OperatorNode *> general_children_;
};