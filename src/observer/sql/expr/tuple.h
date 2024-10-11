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
// Created by Wangyunlai on 2021/5/14.
//

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "common/log/log.h"
#include "sql/expr/expression.h"
#include "sql/expr/tuple_cell.h"
#include "sql/parser/parse.h"
#include "common/value.h"
#include "storage/record/record.h"

class Table;

/**
 * @defgroup Tuple
 * @brief Tuple 元组，表示一行数据，当前返回客户端时使用
 * @details
 * tuple是一种可以嵌套的数据结构。
 * 比如select t1.a+t2.b from t1, t2;
 * 需要使用下面的结构表示：
 * @code {.cpp}
 *  Project(t1.a+t2.b)
 *        |
 *      Joined
 *      /     \
 *   Row(t1) Row(t2)
 * @endcode
 * TODO 一个类拆分成一个文件，并放到单独的目录中
 */

/**
 * @brief 元组的结构，包含哪些字段(这里成为Cell)，每个字段的说明
 * @ingroup Tuple
 */
class TupleSchema
{
public:
  void append_cell(const TupleCellSpec &cell) { cells_.push_back(cell); }
  void append_cell(const char *table, const char *field) { append_cell(TupleCellSpec(table, field)); }
  void append_cell(const char *alias) { append_cell(TupleCellSpec(alias)); }
  int  cell_num() const { return static_cast<int>(cells_.size()); }

  const TupleCellSpec &cell_at(int i) const { return cells_[i]; }

private:
  std::vector<TupleCellSpec> cells_;
};

/**
 * @brief 元组的抽象描述
 * @ingroup Tuple
 */
class Tuple
{
public:
  Tuple()          = default;
  virtual ~Tuple() = default;

  /**
   * @brief 获取元组中的Cell的个数
   * @details 个数应该与tuple_schema一致
   */
  virtual int cell_num() const = 0;

  /**
   * @brief 获取指定位置的Cell
   *
   * @param index 位置
   * @param[out] cell  返回的Cell
   */
  virtual RC cell_at(int index, Value &cell) const = 0;

  virtual RC spec_at(int index, TupleCellSpec &spec) const = 0;

  /**
   * @brief 根据cell的描述，获取cell的值
   *
   * @param spec cell的描述
   * @param[out] cell 返回的cell
   */
  virtual RC find_cell(const TupleCellSpec &spec, Value &cell) const = 0;

  virtual std::string to_string() const
  {
    std::string str;
    const int   cell_num = this->cell_num();
    for (int i = 0; i < cell_num - 1; i++) {
      Value cell;
      cell_at(i, cell);
      str += cell.to_string();
      str += ", ";
    }

    if (cell_num > 0) {
      Value cell;
      cell_at(cell_num - 1, cell);
      str += cell.to_string();
    }
    return str;
  }

  virtual RC compare(const Tuple &other, int &result) const
  {
    RC rc = RC::SUCCESS;

    const int this_cell_num  = this->cell_num();
    const int other_cell_num = other.cell_num();
    if (this_cell_num < other_cell_num) {
      result = -1;
      return rc;
    }
    if (this_cell_num > other_cell_num) {
      result = 1;
      return rc;
    }

    Value this_value;
    Value other_value;
    for (int i = 0; i < this_cell_num; i++) {
      rc = this->cell_at(i, this_value);
      if (OB_FAIL(rc)) {
        return rc;
      }

      rc = other.cell_at(i, other_value);
      if (OB_FAIL(rc)) {
        return rc;
      }

      result = this_value.compare(other_value);
      if (0 != result) {
        return rc;
      }
    }

    result = 0;
    return rc;
  }
};

/**
 * @brief 一行数据的元组
 * @ingroup Tuple
 * @details 直接就是获取表中的一条记录
 */
class RowTuple : public Tuple
{
public:
  RowTuple() = default;
  virtual ~RowTuple()
  {
    for (FieldExpr *spec : speces_) {
      delete spec;
    }
    speces_.clear();
  }

  void set_record(Record *record) { this->record_ = record; }

  void set_schema(const Table *table, const std::vector<FieldMeta> *fields)
  {
    table_ = table;
    // fix:join当中会多次调用右表的open,open当中会调用set_scheme，从而导致tuple当中会存储
    // 很多无意义的field和value，因此需要先clear掉
    for (FieldExpr *spec : speces_) {
      delete spec;
    }
    this->speces_.clear();
    this->speces_.reserve(fields->size());
    for (const FieldMeta &field : *fields) {
      speces_.push_back(new FieldExpr(table, &field));
    }
  }

  int cell_num() const override { return speces_.size(); }

  RC cell_at(int index, Value &cell) const override
  {
    if (index < 0 || index >= static_cast<int>(speces_.size())) {
      LOG_WARN("invalid argument. index=%d", index);
      return RC::INVALID_ARGUMENT;
    }

    FieldExpr       *field_expr = speces_[index];
    const FieldMeta *field_meta = field_expr->field().meta();
    cell.set_type(field_meta->type());
    cell.set_data(this->record_->data() + field_meta->offset(), field_meta->len());
    return RC::SUCCESS;
  }

  RC spec_at(int index, TupleCellSpec &spec) const override
  {
    const Field &field = speces_[index]->field();
    spec               = TupleCellSpec(table_->name(), field.field_name());
    return RC::SUCCESS;
  }

  RC find_cell(const TupleCellSpec &spec, Value &cell) const override
  {
    const char *table_name = spec.table_name();
    const char *field_name = spec.field_name();
    if (0 != strcmp(table_name, table_->name())) {
      return RC::NOTFOUND;
    }

    for (size_t i = 0; i < speces_.size(); ++i) {
      const FieldExpr *field_expr = speces_[i];
      const Field     &field      = field_expr->field();
      if (0 == strcmp(field_name, field.field_name())) {
        return cell_at(i, cell);
      }
    }
    return RC::NOTFOUND;
  }

#if 0
  RC cell_spec_at(int index, const TupleCellSpec *&spec) const override
  {
    if (index < 0 || index >= static_cast<int>(speces_.size())) {
      LOG_WARN("invalid argument. index=%d", index);
      return RC::INVALID_ARGUMENT;
    }
    spec = speces_[index];
    return RC::SUCCESS;
  }
#endif

  Record &record() { return *record_; }

  const Record &record() const { return *record_; }

private:
  Record                  *record_ = nullptr;
  const Table             *table_  = nullptr;
  std::vector<FieldExpr *> speces_;
};

/**
 * @brief 从一行数据中，选择部分字段组成的元组，也就是投影操作
 * @ingroup Tuple
 * @details 一般在select语句中使用。
 * 投影也可以是很复杂的操作，比如某些字段需要做类型转换、重命名、表达式运算、函数计算等。
 */
class ProjectTuple : public Tuple
{
public:
  ProjectTuple()          = default;
  virtual ~ProjectTuple() = default;

  void set_expressions(std::vector<std::unique_ptr<Expression>> &&expressions)
  {
    expressions_ = std::move(expressions);
  }

  auto get_expressions() const -> const std::vector<std::unique_ptr<Expression>> & { return expressions_; }

  void set_tuple(Tuple *tuple) { this->tuple_ = tuple; }

  int cell_num() const override { return static_cast<int>(expressions_.size()); }

  RC cell_at(int index, Value &cell) const override
  {
    if (index < 0 || index >= cell_num()) {
      return RC::INTERNAL;
    }
    if (tuple_ == nullptr) {
      return RC::INTERNAL;
    }

    Expression *expr = expressions_[index].get();
    return expr->get_value(*tuple_, cell);
  }

  RC spec_at(int index, TupleCellSpec &spec) const override
  {
    spec = TupleCellSpec(expressions_[index]->name());
    return RC::SUCCESS;
  }

  RC find_cell(const TupleCellSpec &spec, Value &cell) const override { return tuple_->find_cell(spec, cell); }

#if 0
  RC cell_spec_at(int index, const TupleCellSpec *&spec) const override
  {
    if (index < 0 || index >= static_cast<int>(speces_.size())) {
      return RC::NOTFOUND;
    }
    spec = speces_[index];
    return RC::SUCCESS;
  }
#endif
private:
  std::vector<std::unique_ptr<Expression>> expressions_;
  Tuple                                   *tuple_ = nullptr;
};

/**
 * @brief 一些常量值组成的Tuple
 * @ingroup Tuple
 * TODO 使用单独文件
 */
class ValueListTuple : public Tuple
{
public:
  ValueListTuple()          = default;
  virtual ~ValueListTuple() = default;

  void set_names(const std::vector<TupleCellSpec> &specs) { specs_ = specs; }
  void set_cells(const std::vector<Value> &cells) { cells_ = cells; }

  virtual int cell_num() const override { return static_cast<int>(cells_.size()); }

  virtual RC cell_at(int index, Value &cell) const override
  {
    if (index < 0 || index >= cell_num()) {
      return RC::NOTFOUND;
    }

    cell = cells_[index];
    return RC::SUCCESS;
  }

  RC spec_at(int index, TupleCellSpec &spec) const override
  {
    if (index < 0 || index >= cell_num()) {
      return RC::NOTFOUND;
    }

    spec = specs_[index];
    return RC::SUCCESS;
  }

  virtual RC find_cell(const TupleCellSpec &spec, Value &cell) const override
  {
    ASSERT(cells_.size() == specs_.size(), "cells_.size()=%d, specs_.size()=%d", cells_.size(), specs_.size());

    const int size = static_cast<int>(specs_.size());
    for (int i = 0; i < size; i++) {
      if (specs_[i].equals(spec)) {
        cell = cells_[i];
        return RC::SUCCESS;
      }
    }
    return RC::NOTFOUND;
  }

  static RC make(const Tuple &tuple, ValueListTuple &value_list)
  {
    const int cell_num = tuple.cell_num();
    for (int i = 0; i < cell_num; i++) {
      Value cell;
      RC    rc = tuple.cell_at(i, cell);
      if (OB_FAIL(rc)) {
        return rc;
      }

      TupleCellSpec spec;
      rc = tuple.spec_at(i, spec);
      if (OB_FAIL(rc)) {
        return rc;
      }

      value_list.cells_.push_back(cell);
      value_list.specs_.push_back(spec);
    }
    return RC::SUCCESS;
  }

private:
  std::vector<Value>         cells_;
  std::vector<TupleCellSpec> specs_;
};

/**
 * @brief 将两个tuple合并为一个tuple
 * @ingroup Tuple
 * @details 在join算子中使用
 * TODO replace with composite tuple
 */
class JoinedTuple : public Tuple
{
public:
  JoinedTuple()          = default;
  virtual ~JoinedTuple() = default;

  void set_left(Tuple *left) { left_ = left; }
  void set_right(Tuple *right) { right_ = right; }

  int cell_num() const override { return left_->cell_num() + right_->cell_num(); }

  RC cell_at(int index, Value &value) const override
  {
    const int left_cell_num = left_->cell_num();
    if (index >= 0 && index < left_cell_num) {
      return left_->cell_at(index, value);
    }

    if (index >= left_cell_num && index < left_cell_num + right_->cell_num()) {
      return right_->cell_at(index - left_cell_num, value);
    }

    return RC::NOTFOUND;
  }

  RC spec_at(int index, TupleCellSpec &spec) const override
  {
    const int left_cell_num = left_->cell_num();
    if (index >= 0 && index < left_cell_num) {
      return left_->spec_at(index, spec);
    }

    if (index >= left_cell_num && index < left_cell_num + right_->cell_num()) {
      return right_->spec_at(index - left_cell_num, spec);
    }

    return RC::NOTFOUND;
  }

  RC find_cell(const TupleCellSpec &spec, Value &value) const override
  {
    RC rc = left_->find_cell(spec, value);
    if (rc == RC::SUCCESS || rc != RC::NOTFOUND) {
      return rc;
    }

    return right_->find_cell(spec, value);
  }

private:
  Tuple *left_  = nullptr;
  Tuple *right_ = nullptr;
};
