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
// Created by Wangyunlai on 2022/07/05.
//

#pragma once

#include <string.h>
#include <memory>
#include "storage/common/field.h"
#include "sql/expr/tuple_cell.h"

class Tuple;

enum class ExprType {
  NONE,
  FIELD,
  VALUE,
  CAST,
  COMPARISON,
  CONJUNCTION,
};

class Expression
{
public: 
  Expression() = default;
  virtual ~Expression() = default;
  
  virtual RC get_value(const Tuple &tuple, TupleCell &cell) const = 0;
  virtual ExprType type() const = 0;
  virtual AttrType value_type() const = 0;  
};

class FieldExpr : public Expression
{
public:
  FieldExpr() = default;
  FieldExpr(const Table *table, const FieldMeta *field) : field_(table, field)
  {}
  FieldExpr(const Field &field) : field_(field)
  {}

  virtual ~FieldExpr() = default;

  ExprType type() const override
  {
    return ExprType::FIELD;
  }
  AttrType value_type() const override
  {
    return field_.attr_type();
  }

  Field &field()
  {
    return field_;
  }

  const Field &field() const
  {
    return field_;
  }

  const char *table_name() const
  {
    return field_.table_name();
  }

  const char *field_name() const
  {
    return field_.field_name();
  }

  RC get_value(const Tuple &tuple, TupleCell &cell) const override;
private:
  Field field_;
};

class ValueExpr : public Expression
{
public:
  ValueExpr() = default;
  ValueExpr(const Value &value) : tuple_cell_(value.type, (char *)value.data)
  {
    if (value.type == CHARS) {
      tuple_cell_.set_data((char *)value.data, strlen((const char *)value.data));
    }
  }
  ValueExpr(const TupleCell &cell) : tuple_cell_(cell)
  {}

  virtual ~ValueExpr() = default;

  RC get_value(const Tuple &tuple, TupleCell & cell) const override;

  ExprType type() const override
  {
    return ExprType::VALUE;
  }

  AttrType value_type() const override
  {
    return tuple_cell_.attr_type();
  }

  void get_tuple_cell(TupleCell &cell) const {
    cell = tuple_cell_;
  }

  const TupleCell &get_tuple_cell() const {
    return tuple_cell_;
  }

private:
  TupleCell tuple_cell_;
};

class CastExpr : public Expression
{
public: 
  CastExpr(std::unique_ptr<Expression> child, AttrType cast_type);
  virtual ~CastExpr();

  ExprType type() const override { return ExprType::CAST; }
  RC get_value(const Tuple &tuple, TupleCell &cell) const override;
  AttrType value_type() const override
  {
    return cast_type_;
  }

  std::unique_ptr<Expression> &child()  { return child_; }
private:
  std::unique_ptr<Expression> child_;
  AttrType cast_type_;
};

class ComparisonExpr : public Expression
{
public:
  ComparisonExpr(CompOp comp, std::unique_ptr<Expression> left, std::unique_ptr<Expression> right);
  virtual ~ComparisonExpr();
  
  ExprType type() const override { return ExprType::COMPARISON; }
  RC get_value(const Tuple &tuple, TupleCell &cell) const override;
  AttrType value_type() const override { return BOOLEANS; }

  std::unique_ptr<Expression> &left() { return left_; }
  std::unique_ptr<Expression> &right() { return right_; }

  RC try_get_value(TupleCell &cell) const;

  /**
   * compare the two tuple cells
   * @param value the result of comparison
   */
  RC compare_tuple_cell(const TupleCell &left, const TupleCell &right, bool &value) const;

private:
  CompOp comp_;
  std::unique_ptr<Expression> left_;
  std::unique_ptr<Expression> right_;
};

class ConjunctionExpr : public Expression
{
public:
  enum class Type
  {
    AND,
    OR,
  };
  
public:
  ConjunctionExpr(Type type, std::vector<std::unique_ptr<Expression>> &children);
  virtual ~ConjunctionExpr() = default;

  ExprType type() const override { return ExprType::CONJUNCTION; }
  AttrType value_type() const override { return BOOLEANS; }
  RC get_value(const Tuple &tuple, TupleCell &cell) const override;

  std::vector<std::unique_ptr<Expression>> &children() { return children_; }
private:
  Type  conjunction_type_;
  std::vector<std::unique_ptr<Expression>> children_;
};
