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
// Created by Meiyi & Wangyunlai on 2021/5/14.
//

#pragma once

#include <memory>
#include <vector>

#include "common/log/log.h"
#include "sql/parser/parse.h"
#include "sql/executor/value.h"
#include "storage/common/field.h"
#include "storage/common/record.h"

class Table;

class TupleCellSpec
{
  
};

class Tuple
{
public:
  Tuple() = default;
  virtual ~Tuple() = default;

  virtual int cell_num() const = 0; 
  virtual RC  cell_at(int index, TupleCell &cell) const = 0;

  //virtual RC  cell_spec_at(int index, TupleCellSpec &spec) const;
};

class RowTuple : public Tuple
{
public:
  RowTuple() = default;
  virtual ~RowTuple() = default;
  
  void set_record(Record *record)
  {
    this->record_ = record;
  }

  void set_table(Table *table)
  {
    this->table_ = table;
  }

  void set_schema(const std::vector<FieldMeta> *fields)
  {
    this->fields_ = fields;
  }

  int cell_num() const override
  {
    return fields_->size();
  }

  RC cell_at(int index, TupleCell &cell) const override
  {
    if (index < 0 || index >= fields_->size()) {
      LOG_WARN("invalid argument. index=%d", index);
      return RC::INVALID_ARGUMENT;
    }

    const FieldMeta &field_meta = (*fields_)[index];
    cell.set_table(table_);
    cell.set_type(field_meta.type());
    cell.set_data(this->record_->data() + field_meta.offset());
    return RC::SUCCESS;
  }

  RC cell_spec_at(int index, TupleCellSpec &spec) const
  {
    if (index < 0 || index >= fields_->size()) {
      LOG_WARN("invalid argument. index=%d", index);
      return RC::INVALID_ARGUMENT;
    }
    const FieldMeta &field_meta = (*fields_)[index];
    // TODO
    return RC::SUCCESS;
  }

  Record &record()
  {
    return *record_;
  }

  const Record &record() const
  {
    return *record_;
  }
private:
  Record *record_ = nullptr;
  Table *table_ = nullptr;
  const std::vector<FieldMeta> *fields_ = nullptr;
};

/*
class CompositeTuple : public Tuple
{
public:
private:
  std::vector<Tuple *> tuples_;
};
*/
