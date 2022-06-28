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
// Created by Wangyunlai on 2022/5/22.
//

#pragma once

#include <vector>
#include "rc.h"
#include "sql/parser/parse_defs.h"
#include "sql/stmt/stmt.h"

class Db;
class Table;
class FieldMeta;

class FilterField
{
public:
  FilterField() = default;

  FilterField(Table *table, FieldMeta *field) : table_(table), field_(field)
  {}

  Table *table() const {
    return table_;
  }

  const FieldMeta *field() const {
    return field_;
  }

  void set_table(Table *table) {
    table_ = table;
  }
  void set_field(const FieldMeta *field) {
    field_ = field;
  }
private:
  Table *table_ = nullptr;
  const FieldMeta *field_ = nullptr;
};

class FilterItem
{
public:
  FilterItem() = default;

  void set_field(Table *table, const FieldMeta *field) {
    is_attr_ = true;
    field_.set_table(table);
    field_.set_field(field);
  }

  void set_value(const Value &value) {
    is_attr_ = false;
    value_ = value;
  }

  bool is_attr() const {
    return is_attr_;
  }

  const FilterField &field() const {
    return field_;
  }

  const Value &value() const {
    return value_;
  }

private:
  bool is_attr_ = false; // is an attribute or a value
  FilterField field_;
  Value value_;
};

class FilterUnit
{
public:
  FilterUnit() = default;
  
  void set_comp(CompOp comp) {
    comp_ = comp;
  }

  CompOp comp() const {
    return comp_;
  }
  FilterItem &left() {
    return left_;
  }
  FilterItem &right() {
    return right_;
  }
  const FilterItem &left() const {
    return left_;
  }
  const FilterItem &right() const {
    return right_;
  }
private:
  CompOp comp_;
  FilterItem left_;
  FilterItem right_;
};

class FilterStmt 
{
public:

  FilterStmt() = default;

public:
  const std::vector<FilterUnit> &filter_units() const
  {
    return filter_units_;
  }

public:
  static RC create(Db *db, Table *default_table,
			const Condition *conditions, int condition_num,
			FilterStmt *&stmt);

  static RC create_filter_unit(Db *db, Table *default_table, const Condition &condition, FilterUnit &filter_unit);

private:
  std::vector<FilterUnit>  filter_units_; // 默认当前都是AND关系
};
