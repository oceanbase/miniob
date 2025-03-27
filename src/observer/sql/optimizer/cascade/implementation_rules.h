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

#include "sql/optimizer/cascade/rules.h"

/**
 * Rule transforms Logical Scan -> Physical Scan
 */
class LogicalGetToPhysicalSeqScan : public Rule
{
public:
  LogicalGetToPhysicalSeqScan();

  void transform(OperatorNode *input, std::vector<std::unique_ptr<OperatorNode>> *transformed,
      OptimizerContext *context) const override;
};

// TODO: support index scan
// class LogicalGetToPhysicalIndexScan : public Rule {
// };

/**
 * Rule transforms Logical Projection -> Physical Projection
 */
class LogicalProjectionToProjection : public Rule
{
public:
  LogicalProjectionToProjection();

  void transform(OperatorNode *input, std::vector<std::unique_ptr<OperatorNode>> *transformed,
      OptimizerContext *context) const override;
};

/**
 * Rule transforms Logical Insert -> Physical Insert
 */
class LogicalInsertToInsert : public Rule
{
public:
  LogicalInsertToInsert();

  void transform(OperatorNode *input, std::vector<std::unique_ptr<OperatorNode>> *transformed,
      OptimizerContext *context) const override;
};

/**
 * Rule transforms Logical explain -> Physical explain
 */
class LogicalExplainToExplain : public Rule
{
public:
  LogicalExplainToExplain();

  void transform(OperatorNode *input, std::vector<std::unique_ptr<OperatorNode>> *transformed,
      OptimizerContext *context) const override;
};

/**
 * Rule transforms Logical calculate -> Physical calculate
 */
class LogicalCalcToCalc : public Rule
{
public:
  LogicalCalcToCalc();

  void transform(OperatorNode *input, std::vector<std::unique_ptr<OperatorNode>> *transformed,
      OptimizerContext *context) const override;
};

/**
 * Rule transforms Logical delete -> Physical delete
 */
class LogicalDeleteToDelete : public Rule
{
public:
  LogicalDeleteToDelete();

  void transform(OperatorNode *input, std::vector<std::unique_ptr<OperatorNode>> *transformed,
      OptimizerContext *context) const override;
};

/**
 * Rule transforms Logical predicate -> Physical predicate
 * TODO: In practice, this rule may not be used and can be removed
 */
class LogicalPredicateToPredicate : public Rule
{
public:
  LogicalPredicateToPredicate();

  void transform(OperatorNode *input, std::vector<std::unique_ptr<OperatorNode>> *transformed,
      OptimizerContext *context) const override;
};

/**
 * Rule transforms Logical Groupby -> Physical Aggregation(Scalar Groupby)
 * TODO: currently group by is competition problem, so we don't implement this rule
 */
// class LogicalGroupByToAggregation : public Rule {
//  public:
//   LogicalGroupByToAggregation();

//   void transform(OperatorNode* input,
//                          std::vector<std::unique_ptr<OperatorNode>> *transformed,
//                          OptimizerContext *context) const override;
// };

/**
 * Rule transforms Logical GroupBy -> Physical GroupBy(Hash GroupBy)
 * TODO: currently group by is competition problem, so we don't implement this rule
 */
// class LogicalGroupByToHashGroupBy : public Rule {
//  public:
//   LogicalGroupByToHashGroupBy();

//   void transform(OperatorNode* input,
//                          std::vector<std::unique_ptr<OperatorNode>> *transformed,
//                          OptimizerContext *context) const override;
// };