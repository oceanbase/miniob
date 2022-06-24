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
// Created by WangYunlai on 2021/6/7.
//

#pragma once

#include "sql/executor/predicate.h"
#include "sql/executor/operator.h"
#include "storage/common/record_manager.h"
#include "rc.h"

class Table;
class Predicate;

class TableScanOperator : public Operator
{
public:
  TableScanOperator(Table *table)
    : table_(table), predicate_(nullptr)
  {}

  TableScanOperator(Table *table, Predicate *pred)
    : table_(table), predicate_(pred)
  {}

  virtual ~TableScanOperator() = default;

  RC open() override;
  RC next() override;
  RC close() override;

  RC current_record(Record &record) override;
private:
  Table *table_ = nullptr;
  RecordFileScanner record_scanner_;
  Predicate *predicate_ = nullptr;
  Record current_record_;
};
