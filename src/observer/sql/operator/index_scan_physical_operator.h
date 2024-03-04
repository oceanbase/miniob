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
// Created by Wangyunlai on 2022/07/08.
//

#pragma once

#include "sql/expr/tuple.h"
#include "sql/operator/physical_operator.h"
#include "storage/record/record_manager.h"

/**
 * @brief 索引扫描物理算子
 * @ingroup PhysicalOperator
 */
class IndexScanPhysicalOperator : public PhysicalOperator
{
public:
  IndexScanPhysicalOperator(Table *table, Index *index, bool readonly, const Value *left_value, bool left_inclusive,
      const Value *right_value, bool right_inclusive);

  virtual ~IndexScanPhysicalOperator() = default;

  PhysicalOperatorType type() const override { return PhysicalOperatorType::INDEX_SCAN; }

  std::string param() const override;

  RC open(Trx *trx) override;
  RC next() override;
  RC close() override;

  Tuple *current_tuple() override;

  void set_predicates(std::vector<std::unique_ptr<Expression>> &&exprs);

private:
  // 与TableScanPhysicalOperator代码相同，可以优化
  RC filter(RowTuple &tuple, bool &result);

private:
  Trx               *trx_            = nullptr;
  Table             *table_          = nullptr;
  Index             *index_          = nullptr;
  bool               readonly_       = false;
  IndexScanner      *index_scanner_  = nullptr;
  RecordFileHandler *record_handler_ = nullptr;

  RecordPageHandler record_page_handler_;
  Record            current_record_;
  RowTuple          tuple_;

  Value left_value_;
  Value right_value_;
  bool  left_inclusive_  = false;
  bool  right_inclusive_ = false;

  std::vector<std::unique_ptr<Expression>> predicates_;
};
