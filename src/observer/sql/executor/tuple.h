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
#include "storage/common/field.h"
#include "storage/common/record.h"

class Table;

class TupleCellSpec
{
public:
  TupleCellSpec() = default;
  TupleCellSpec(const char *table_name, const char *field_name, const char *alias)
    : table_name_(table_name), field_name_(field_name), alias_(alias)
  {}

  void set_table_name(const char *table_name)
  {
    this->table_name_ = table_name;
  }

  void set_field_name(const char *field_name)
  {
    this->field_name_ = field_name;
  }

  void set_alias(const char *alias)
  {
    this->alias_ = alias;
  }
  const char *table_name() const
  {
    return table_name_;
  }
  const char *field_name() const
  {
    return field_name_;
  }
  const char *alias() const
  {
    return alias_;
  }

  bool is_same_cell(const TupleCellSpec &other) const
  {
    return 0 == strcmp(this->table_name_, other.table_name_) &&
      0 == strcmp(this->field_name_, other.field_name_);
  }

private:
  // TODO table and field cannot describe all scenerio, should be expression
  const char *table_name_ = nullptr;
  const char *field_name_ = nullptr;
  const char *alias_ = nullptr;
};

class Tuple
{
public:
  Tuple() = default;
  virtual ~Tuple() = default;

  virtual int cell_num() const = 0; 
  virtual RC  cell_at(int index, TupleCell &cell) const = 0;
  virtual RC  find_cell(const TupleCellSpec &spec, TupleCell &cell) const = 0;

  virtual RC  cell_spec_at(int index, TupleCellSpec &spec) const = 0;
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

  RC find_cell(const TupleCellSpec &spec, TupleCell &cell) const override
  {
    const char *table_name = spec.table_name();
    if (0 != strcmp(table_name, table_->name())) {
      return RC::NOTFOUND;
    }

    const char *field_name = spec.field_name();
    for (int i = 0; i < fields_->size(); ++i) {
      const FieldMeta &field_meta = (*fields_)[i];
      if (0 == strcmp(field_name, field_meta.name())) {
	return cell_at(i, cell);
      }
    }
    return RC::NOTFOUND;
  }

  RC cell_spec_at(int index, TupleCellSpec &spec) const override
  {
    if (index < 0 || index >= fields_->size()) {
      LOG_WARN("invalid argument. index=%d", index);
      return RC::INVALID_ARGUMENT;
    }
    const FieldMeta &field_meta = (*fields_)[index];
    spec.set_table_name(table_->name());
    spec.set_field_name(field_meta.name());
    spec.set_alias(field_meta.name());
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
  int cell_num() const override; 
  RC  cell_at(int index, TupleCell &cell) const = 0;
private:
  int cell_num_ = 0;
  std::vector<Tuple *> tuples_;
};
*/

class ProjectTuple : public Tuple
{
public:
  ProjectTuple() = default;

  void set_tuple(Tuple *tuple)
  {
    this->tuple_ = tuple;
  }

  void add_cell_spec(const TupleCellSpec &spec)
  {
    speces_.push_back(spec);
  }
  int cell_num() const override
  {
    return speces_.size();
  }

  RC cell_at(int index, TupleCell &cell) const override
  {
    if (index < 0 || index >= speces_.size()) {
      return RC::GENERIC_ERROR;
    }
    if (tuple_ == nullptr) {
      return RC::GENERIC_ERROR;
    }

    const TupleCellSpec &spec = speces_[index]; // TODO better: add mapping between projection and raw tuple
    return tuple_->find_cell(spec, cell);
  }

  RC find_cell(const TupleCellSpec &spec, TupleCell &cell) const override
  {
    return tuple_->find_cell(spec, cell);
  }
  RC cell_spec_at(int index, TupleCellSpec &spec) const override
  {
    if (index < 0 || index >= speces_.size()) {
      return RC::NOTFOUND;
    }
    spec = speces_[index];
    return RC::SUCCESS;
  }
private:
  std::vector<TupleCellSpec> speces_;
  Tuple *tuple_ = nullptr;
};
