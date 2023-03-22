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
// Created by Wangyunlai on 2022/07/08.
//

#pragma once

#include "sql/operator/physical_operator.h"
#include "sql/expr/tuple.h"

class IndexScanPhysicalOperator : public PhysicalOperator {
public:
  IndexScanPhysicalOperator(const Table *table, Index *index, const TupleCell *left_cell, bool left_inclusive,
      const TupleCell *right_cell, bool right_inclusive);

  virtual ~IndexScanPhysicalOperator() = default;

  PhysicalOperatorType type() const override
  {
    return PhysicalOperatorType::INDEX_SCAN;
  }

  std::string param() const override;

  RC open() override;
  RC next() override;
  RC close() override;

  Tuple *current_tuple() override;

  void set_predicates(std::vector<std::unique_ptr<Expression>> &&exprs);

private:
  // 与TableScanPhysicalOperator代码相同，可以优化
  RC filter(RowTuple &tuple, bool &result);

private:
  const Table *table_ = nullptr;
  Index *index_ = nullptr;
  IndexScanner *index_scanner_ = nullptr;
  RecordFileHandler *record_handler_ = nullptr;

  Record current_record_;
  RowTuple tuple_;

  TupleCell left_cell_;
  TupleCell right_cell_;
  bool left_inclusive_;
  bool right_inclusive_;

  std::vector<std::unique_ptr<Expression>> predicates_;
};
