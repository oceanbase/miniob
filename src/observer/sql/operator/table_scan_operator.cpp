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
// Created by WangYunlai on 2021/6/9.
//

#include "sql/operator/table_scan_operator.h"
#include "storage/common/table.h"
#include "rc.h"

RC TableScanOperator::open()
{
  RC rc = table_->get_record_scanner(record_scanner_);
  if (rc == RC::SUCCESS) {
    tuple_.set_schema(table_, table_->table_meta().field_metas());
  }
  return rc;
}

RC TableScanOperator::next()
{
  if (!record_scanner_.has_next()) {
    return RC::RECORD_EOF;
  }

  RC rc = record_scanner_.next(current_record_);
  return rc;
}

RC TableScanOperator::close()
{
  return record_scanner_.close_scan();
}

Tuple * TableScanOperator::current_tuple()
{
  tuple_.set_record(&current_record_);
  return &tuple_;
}
// RC TableScanOperator::tuple_cell_spec_at(int index, TupleCellSpec &spec) const
// {
//   return tuple_.cell_spec_at(index, spec);
// }
