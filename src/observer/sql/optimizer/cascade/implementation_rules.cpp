/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "common/log/log.h"
#include "sql/optimizer/cascade/implementation_rules.h"
#include "sql/operator/table_get_logical_operator.h"
#include "sql/operator/table_scan_physical_operator.h"
#include "sql/operator/project_logical_operator.h"
#include "sql/operator/project_physical_operator.h"
#include "sql/operator/insert_logical_operator.h"
#include "sql/operator/insert_physical_operator.h"
#include "sql/operator/explain_logical_operator.h"
#include "sql/operator/explain_physical_operator.h"
#include "sql/operator/calc_logical_operator.h"
#include "sql/operator/calc_physical_operator.h"
#include "sql/operator/delete_logical_operator.h"
#include "sql/operator/delete_physical_operator.h"
#include "sql/operator/predicate_logical_operator.h"
#include "sql/operator/predicate_physical_operator.h"
#include "sql/operator/group_by_logical_operator.h"
#include "sql/operator/scalar_group_by_physical_operator.h"
#include "sql/operator/hash_group_by_physical_operator.h"

// -------------------------------------------------------------------------------------------------
// PhysicalSeqScan
// -------------------------------------------------------------------------------------------------
LogicalGetToPhysicalSeqScan::LogicalGetToPhysicalSeqScan() {
  type_ = RuleType::GET_TO_SEQ_SCAN;
  match_pattern_ = unique_ptr<Pattern>(new Pattern(OpType::LOGICALGET));
}

void LogicalGetToPhysicalSeqScan::transform(OperatorNode* input,
                         std::vector<std::unique_ptr<OperatorNode>> *transformed,
                         OptimizerContext *context) const {
  TableGetLogicalOperator* table_get_oper = dynamic_cast<TableGetLogicalOperator*>(input);

  vector<unique_ptr<Expression>> &log_preds = table_get_oper->predicates();
  vector<unique_ptr<Expression>> phys_preds;
  for (auto &pred : log_preds) {
    phys_preds.push_back(pred->copy());
  }

  Table *table = table_get_oper->table();
  auto table_scan_oper = new TableScanPhysicalOperator(table, table_get_oper->read_write_mode());
  table_scan_oper->set_predicates(std::move(phys_preds));
  auto oper = unique_ptr<OperatorNode>(table_scan_oper);

  transformed->emplace_back(std::move(oper));
}

// -------------------------------------------------------------------------------------------------
//  LogicalProjectionToProjection
// -------------------------------------------------------------------------------------------------
LogicalProjectionToProjection::LogicalProjectionToProjection() {
  type_ = RuleType::PROJECTION_TO_PHYSOCAL;
  match_pattern_ = unique_ptr<Pattern>(new Pattern(OpType::LOGICALPROJECTION));
  auto child = new Pattern(OpType::LEAF);
  match_pattern_->add_child(child);
}

void LogicalProjectionToProjection::transform(OperatorNode* input,
                         std::vector<std::unique_ptr<OperatorNode>> *transformed,
                         OptimizerContext *context) const {
  auto project_oper = dynamic_cast<ProjectLogicalOperator*>(input);
  vector<unique_ptr<LogicalOperator>> &child_opers = project_oper->children();
  ASSERT(child_opers.size() == 1, "only one child is supported for now");

  unique_ptr<PhysicalOperator> child_phy_oper;

  auto project_operator = make_unique<ProjectPhysicalOperator>(std::move(project_oper->expressions()));
  if (project_operator) {
    project_operator->add_general_child(child_opers.front().get());
  }

  transformed->emplace_back(std::move(project_operator));
}

// -------------------------------------------------------------------------------------------------
// PhysicalInsert
// -------------------------------------------------------------------------------------------------
LogicalInsertToInsert::LogicalInsertToInsert() {
  type_ = RuleType::INSERT_TO_PHYSICAL;
  match_pattern_ = unique_ptr<Pattern>(new Pattern(OpType::LOGICALINSERT));
}


void LogicalInsertToInsert::transform(OperatorNode* input,
                         std::vector<std::unique_ptr<OperatorNode>> *transformed,
                         OptimizerContext *context) const {
  InsertLogicalOperator* insert_oper = dynamic_cast<InsertLogicalOperator*>(input);

  Table                  *table           = insert_oper->table();
  vector<Value>          &values          = insert_oper->values();
  auto insert_phy_oper = make_unique<InsertPhysicalOperator>(table, std::move(values));

  transformed->emplace_back(std::move(insert_phy_oper));
}

// -------------------------------------------------------------------------------------------------
// PhysicalExplain
// -------------------------------------------------------------------------------------------------
LogicalExplainToExplain::LogicalExplainToExplain()
{
  type_ = RuleType::EXPLAIN_TO_PHYSICAL;
  match_pattern_ = unique_ptr<Pattern>(new Pattern(OpType::LOGICALEXPLAIN));
  auto child = new Pattern(OpType::LEAF);
  match_pattern_->add_child(child);
}

void LogicalExplainToExplain::transform(OperatorNode* input,
                         std::vector<std::unique_ptr<OperatorNode>> *transformed,
                         OptimizerContext *context) const
{
  auto explain_oper = dynamic_cast<ExplainLogicalOperator*>(input);
  unique_ptr<PhysicalOperator> explain_physical_oper(new ExplainPhysicalOperator());
  for (auto &child : explain_oper->children()) {
    explain_physical_oper->add_general_child(child.get());
  }

  transformed->emplace_back(std::move(explain_physical_oper));
}

// -------------------------------------------------------------------------------------------------
// PhysicalCalc
// -------------------------------------------------------------------------------------------------
LogicalCalcToCalc::LogicalCalcToCalc()
{
  type_ = RuleType::CALC_TO_PHYSICAL;
  match_pattern_ = unique_ptr<Pattern>(new Pattern(OpType::LOGICALCALCULATE));
}

void LogicalCalcToCalc::transform(OperatorNode* input,
                         std::vector<std::unique_ptr<OperatorNode>> *transformed,
                         OptimizerContext *context) const
{
  auto calc_oper = dynamic_cast<CalcLogicalOperator*>(input);
  unique_ptr<CalcPhysicalOperator> calc_phys_oper(new CalcPhysicalOperator(std::move(calc_oper->expressions())));

  transformed->emplace_back(std::move(calc_phys_oper));
}

// -------------------------------------------------------------------------------------------------
// PhysicalDelete
// -------------------------------------------------------------------------------------------------
LogicalDeleteToDelete::LogicalDeleteToDelete()
{
  type_ = RuleType::DELETE_TO_PHYSICAL;
  match_pattern_ = unique_ptr<Pattern>(new Pattern(OpType::LOGICALDELETE));
  auto child = new Pattern(OpType::LEAF);
  match_pattern_->add_child(child);
}

void LogicalDeleteToDelete::transform(OperatorNode* input,
                         std::vector<std::unique_ptr<OperatorNode>> *transformed,
                         OptimizerContext *context) const
{
  auto delete_oper = dynamic_cast<DeleteLogicalOperator*>(input);

  auto delete_phys_oper = unique_ptr<PhysicalOperator>(new DeletePhysicalOperator(delete_oper->table()));
  for (auto &child : delete_oper->children()) {
    delete_phys_oper->add_general_child(child.get());
  }

  transformed->emplace_back(std::move(delete_phys_oper));
}

// -------------------------------------------------------------------------------------------------
// Physical Predicate
// -------------------------------------------------------------------------------------------------
LogicalPredicateToPredicate::LogicalPredicateToPredicate()
{
  type_ = RuleType::PREDICATE_TO_PHYSICAL;
  match_pattern_ = unique_ptr<Pattern>(new Pattern(OpType::LOGICALFILTER));
  auto child = new Pattern(OpType::LEAF);
  match_pattern_->add_child(child);
}

void LogicalPredicateToPredicate::transform(OperatorNode* input,
                         std::vector<std::unique_ptr<OperatorNode>> *transformed,
                         OptimizerContext *context) const
{
  auto predicate_oper = dynamic_cast<PredicateLogicalOperator*>(input);

  vector<unique_ptr<Expression>> &expressions = predicate_oper->expressions();
  ASSERT(expressions.size() == 1, "predicate logical operator's children should be 1");

  unique_ptr<Expression> expression = std::move(expressions.front());
  unique_ptr<PhysicalOperator> oper = unique_ptr<PhysicalOperator>(new PredicatePhysicalOperator(std::move(expression)));
  for (auto &child : predicate_oper->children()) {
    oper->add_general_child(child.get());
  }
  transformed->emplace_back(std::move(oper));
}

// -------------------------------------------------------------------------------------------------
// Physical Aggregation
// -------------------------------------------------------------------------------------------------
// LogicalGroupByToAggregation::LogicalGroupByToAggregation()
// {
//   type_ = RuleType::GROUP_BY_TO_PHYSICAL_AGGREGATION;
//   match_pattern_ = unique_ptr<Pattern>(new Pattern(OpType::LOGICALGROUPBY));
//   auto child = new Pattern(OpType::LEAF);
//   match_pattern_->add_child(child);
// }


// void LogicalGroupByToAggregation::transform(OperatorNode* input,
//                          std::vector<std::unique_ptr<OperatorNode>> *transformed,
//                          OptimizerContext *context) const
// {
//   auto groupby_oper = dynamic_cast<GroupByLogicalOperator*>(input);
//   vector<unique_ptr<Expression>> &group_by_expressions = groupby_oper->group_by_expressions();
//   unique_ptr<GroupByPhysicalOperator> groupby_phys_oper;
//   if (group_by_expressions.empty()) {
//     groupby_phys_oper = make_unique<ScalarGroupByPhysicalOperator>(std::move(groupby_oper->aggregate_expressions()));
//   } else {
//     return;
//   }
//   for (auto &child : groupby_oper->children()) {
//     groupby_phys_oper->add_general_child(child.get());
//   }

//   transformed->emplace_back(std::move(groupby_phys_oper));
// }


// -------------------------------------------------------------------------------------------------
// Physical Hash Group By
// -------------------------------------------------------------------------------------------------
// LogicalGroupByToHashGroupBy::LogicalGroupByToHashGroupBy()
// {
//   type_ = RuleType::GROUP_BY_TO_PHYSICL_HASH_GROUP_BY;
//   match_pattern_ = unique_ptr<Pattern>(new Pattern(OpType::LOGICALGROUPBY));
//   auto child = new Pattern(OpType::LEAF);
//   match_pattern_->add_child(child);
// }


// void LogicalGroupByToHashGroupBy::transform(OperatorNode* input,
//                          std::vector<std::unique_ptr<OperatorNode>> *transformed,
//                          OptimizerContext *context) const
// {
//   auto groupby_oper = dynamic_cast<GroupByLogicalOperator*>(input);
//   vector<unique_ptr<Expression>> &group_by_expressions = groupby_oper->group_by_expressions();
//   unique_ptr<GroupByPhysicalOperator> groupby_phys_oper;
//   if (group_by_expressions.empty()) {
//     return;
//   } else {
//     groupby_phys_oper = make_unique<HashGroupByPhysicalOperator>(std::move(groupby_oper->group_by_expressions()),
//         std::move(groupby_oper->aggregate_expressions()));
//   }
//   for (auto &child : groupby_oper->children()) {
//     groupby_phys_oper->add_general_child(child.get());
//   }

//   transformed->emplace_back(std::move(groupby_phys_oper));
// }