/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by WangYunlai on 2022/07/01.
//

#pragma once

#include "sql/expr/tuple.h"
#include "sql/operator/physical_operator.h"

class CalcPhysicalOperator : public PhysicalOperator
{
public:
  CalcPhysicalOperator(std::vector<std::unique_ptr<Expression>> &&expressions)
      : expressions_(std::move(expressions)), tuple_(expressions_)
  {}

  virtual ~CalcPhysicalOperator() = default;

  PhysicalOperatorType type() const override { return PhysicalOperatorType::CALC; }

  std::string name() const override { return "CALC"; }
  std::string param() const override { return ""; }

  RC open(Trx *trx) override { return RC::SUCCESS; }
  RC next() override
  {
    RC rc = RC::SUCCESS;
    if (emitted_) {
      rc = RC::RECORD_EOF;
      return rc;
    }
    emitted_ = true;

    int cell_num = tuple_.cell_num();
    for (int i = 0; i < cell_num; i++) {
      Value value;
      rc = tuple_.cell_at(i, value);
      if (OB_FAIL(rc)) {
        return rc;
      }
    }
    return RC::SUCCESS;
  }
  RC close() override { return RC::SUCCESS; }

  int cell_num() const { return tuple_.cell_num(); }

  Tuple *current_tuple() override { return &tuple_; }

  const std::vector<std::unique_ptr<Expression>> &expressions() const { return expressions_; }

private:
  std::vector<std::unique_ptr<Expression>> expressions_;
  ExpressionTuple                          tuple_;
  bool                                     emitted_ = false;
};
