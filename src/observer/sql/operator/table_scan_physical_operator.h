/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by WangYunlai on 2022/6/7.
//

#pragma once

#include "sql/operator/physical_operator.h"
#include "storage/record/record_manager.h"
#include "rc.h"

class Table;

class TableScanPhysicalOperator : public PhysicalOperator {
public:
  TableScanPhysicalOperator(Table *table) : table_(table)
  {}

  virtual ~TableScanPhysicalOperator() = default;

  std::string param() const override;

  PhysicalOperatorType type() const override
  {
    return PhysicalOperatorType::TABLE_SCAN;
  }

  RC open() override;
  RC next() override;
  RC close() override;

  Tuple *current_tuple() override;

  void set_predicates(std::vector<std::unique_ptr<Expression>> &&exprs);

private:
  RC filter(RowTuple &tuple, bool &result);

private:
  Table *table_ = nullptr;
  RecordFileScanner record_scanner_;
  Record current_record_;
  RowTuple tuple_;
  std::vector<std::unique_ptr<Expression>> predicates_;
};
