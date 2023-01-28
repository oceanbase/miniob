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
#include "common/log/log.h"

class Tuple;

enum class ExprType {
  NONE,
  FIELD,
  VALUE,
  CAST,
  COMPARISON,
  CONJUNCTION,
};

/**
 * 表达式的抽象描述
 * 在SQL的元素中，任何需要得出值的元素都可以使用表达式来描述
 * 比如获取某个字段的值、比较运算、类型转换
 * 当然还有一些当前没有实现的表达式，比如算术运算。
 *
 * 通常表达式的值，是在真实的算子运算过程中，拿到具体的tuple后
 * 才能计算出来真实的值。但是有些表达式可能就表示某一个固定的
 * 值，比如ValueExpr。
 */
class Expression {
public:
  Expression() = default;
  virtual ~Expression() = default;

  /**
   * 根据具体的tuple，来计算当前表达式的值
   */
  virtual RC get_value(const Tuple &tuple, TupleCell &cell) const = 0;

  /**
   * 表达式的类型
   * 可以根据表达式类型来转换为具体的子类
   */
  virtual ExprType type() const = 0;

  /**
   * 表达式值的类型
   */
  virtual AttrType value_type() const = 0;
};

class FieldExpr : public Expression {
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

class ValueExpr : public Expression {
public:
  ValueExpr() = default;
  ValueExpr(const Value &value)
  {
    switch (value.type) {
      case UNDEFINED: {
        ASSERT(false, "value's type is invalid");
      } break;
      case INTS: {
        tuple_cell_.set_int(value.int_value);
      } break;
      case FLOATS: {
        tuple_cell_.set_float(value.float_value);
      } break;
      case CHARS: {
        tuple_cell_.set_string(value.string_value.c_str());
      } break;
      case BOOLEANS: {
        tuple_cell_.set_boolean(value.bool_value);
      } break;
    }
  }
  ValueExpr(const TupleCell &cell) : tuple_cell_(cell)
  {}

  virtual ~ValueExpr() = default;

  RC get_value(const Tuple &tuple, TupleCell &cell) const override;

  ExprType type() const override
  {
    return ExprType::VALUE;
  }

  AttrType value_type() const override
  {
    return tuple_cell_.attr_type();
  }

  void get_tuple_cell(TupleCell &cell) const
  {
    cell = tuple_cell_;
  }

  const TupleCell &get_tuple_cell() const
  {
    return tuple_cell_;
  }

private:
  TupleCell tuple_cell_;
};

class CastExpr : public Expression {
public:
  CastExpr(std::unique_ptr<Expression> child, AttrType cast_type);
  virtual ~CastExpr();

  ExprType type() const override
  {
    return ExprType::CAST;
  }
  RC get_value(const Tuple &tuple, TupleCell &cell) const override;
  AttrType value_type() const override
  {
    return cast_type_;
  }

  std::unique_ptr<Expression> &child()
  {
    return child_;
  }

private:
  std::unique_ptr<Expression> child_;
  AttrType cast_type_;
};

class ComparisonExpr : public Expression {
public:
  ComparisonExpr(CompOp comp, std::unique_ptr<Expression> left, std::unique_ptr<Expression> right);
  virtual ~ComparisonExpr();

  ExprType type() const override
  {
    return ExprType::COMPARISON;
  }
  RC get_value(const Tuple &tuple, TupleCell &cell) const override;
  AttrType value_type() const override
  {
    return BOOLEANS;
  }

  CompOp comp() const
  {
    return comp_;
  }
  std::unique_ptr<Expression> &left()
  {
    return left_;
  }
  std::unique_ptr<Expression> &right()
  {
    return right_;
  }

  /**
   * 尝试在没有tuple的情况下获取当前表达式的值
   * 在优化的时候，可能会使用到
   */
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

/**
 * 多个表达式使用同一种关系(AND或OR)来联结
 * 当前miniob仅有AND操作
 */
class ConjunctionExpr : public Expression {
public:
  enum class Type {
    AND,
    OR,
  };

public:
  ConjunctionExpr(Type type, std::vector<std::unique_ptr<Expression>> &children);
  virtual ~ConjunctionExpr() = default;

  ExprType type() const override
  {
    return ExprType::CONJUNCTION;
  }
  AttrType value_type() const override
  {
    return BOOLEANS;
  }
  RC get_value(const Tuple &tuple, TupleCell &cell) const override;

  Type conjunction_type() const
  {
    return conjunction_type_;
  }

  std::vector<std::unique_ptr<Expression>> &children()
  {
    return children_;
  }

private:
  Type conjunction_type_;
  std::vector<std::unique_ptr<Expression>> children_;
};
