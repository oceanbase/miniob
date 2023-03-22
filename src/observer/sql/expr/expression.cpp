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

#include "sql/expr/expression.h"
#include "sql/expr/tuple.h"

RC FieldExpr::get_value(const Tuple &tuple, TupleCell &cell) const
{
  return tuple.find_cell(TupleCellSpec(table_name(), field_name()), cell);
}

RC ValueExpr::get_value(const Tuple &tuple, TupleCell &cell) const
{
  cell = tuple_cell_;
  return RC::SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
CastExpr::CastExpr(std::unique_ptr<Expression> child, AttrType cast_type)
    : child_(std::move(child)), cast_type_(cast_type)
{}

CastExpr::~CastExpr()
{}

RC CastExpr::get_value(const Tuple &tuple, TupleCell &cell) const
{
  RC rc = child_->get_value(tuple, cell);
  if (rc != RC::SUCCESS) {
    return rc;
  }

  if (child_->value_type() == this->value_type()) {
    return rc;
  }

  switch (cast_type_) {
    case BOOLEANS: {
      bool val = cell.get_boolean();
      cell.set_boolean(val);
    } break;
    default: {
      rc = RC::INTERNAL;
      LOG_WARN("unsupported convert from type %d to %d", child_->value_type(), cast_type_);
    }
  }
  return rc;
}

////////////////////////////////////////////////////////////////////////////////

ComparisonExpr::ComparisonExpr(CompOp comp, std::unique_ptr<Expression> left, std::unique_ptr<Expression> right)
    : comp_(comp), left_(std::move(left)), right_(std::move(right))
{}

ComparisonExpr::~ComparisonExpr()
{}

RC ComparisonExpr::compare_tuple_cell(const TupleCell &left, const TupleCell &right, bool &value) const
{
  RC rc = RC::SUCCESS;
  int cmp_result = left.compare(right);
  value = false;
  switch (comp_) {
    case EQUAL_TO: {
      value = (0 == cmp_result);
    } break;
    case LESS_EQUAL: {
      value = (cmp_result <= 0);
    } break;
    case NOT_EQUAL: {
      value = (cmp_result != 0);
    } break;
    case LESS_THAN: {
      value = (cmp_result < 0);
    } break;
    case GREAT_EQUAL: {
      value = (cmp_result >= 0);
    } break;
    case GREAT_THAN: {
      value = (cmp_result > 0);
    } break;
    default: {
      LOG_WARN("unsupported comparison. %d", comp_);
      rc = RC::GENERIC_ERROR;
    } break;
  }

  return rc;
}

RC ComparisonExpr::try_get_value(TupleCell &cell) const
{
  if (left_->type() == ExprType::VALUE && right_->type() == ExprType::VALUE) {
    ValueExpr *left_value_expr = static_cast<ValueExpr *>(left_.get());
    ValueExpr *right_value_expr = static_cast<ValueExpr *>(right_.get());
    const TupleCell &left_cell = left_value_expr->get_tuple_cell();
    const TupleCell &right_cell = right_value_expr->get_tuple_cell();

    bool value = false;
    RC rc = compare_tuple_cell(left_cell, right_cell, value);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to compare tuple cells. rc=%s", strrc(rc));
    } else {
      cell.set_boolean(value);
    }
    return rc;
  }

  return RC::INVALID_ARGUMENT;
}

RC ComparisonExpr::get_value(const Tuple &tuple, TupleCell &cell) const
{
  TupleCell left_cell;
  TupleCell right_cell;

  RC rc = left_->get_value(tuple, left_cell);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to get value of left expression. rc=%s", strrc(rc));
    return rc;
  }
  rc = right_->get_value(tuple, right_cell);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to get value of right expression. rc=%s", strrc(rc));
    return rc;
  }

  bool value = false;
  rc = compare_tuple_cell(left_cell, right_cell, value);
  if (rc == RC::SUCCESS) {
    cell.set_boolean(value);
  }
  return rc;
}

////////////////////////////////////////////////////////////////////////////////
ConjunctionExpr::ConjunctionExpr(Type type, std::vector<std::unique_ptr<Expression>> &children)
    : conjunction_type_(type), children_(std::move(children))
{}

RC ConjunctionExpr::get_value(const Tuple &tuple, TupleCell &cell) const
{
  RC rc = RC::SUCCESS;
  if (children_.empty()) {
    cell.set_boolean(true);
    return rc;
  }

  TupleCell tmp_cell;
  for (const std::unique_ptr<Expression> &expr : children_) {
    rc = expr->get_value(tuple, tmp_cell);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to get value by child expression. rc=%s", strrc(rc));
      return rc;
    }
    bool value = tmp_cell.get_boolean();
    if ((conjunction_type_ == Type::AND && !value) || (conjunction_type_ == Type::OR && value)) {
      cell.set_boolean(value);
      return rc;
    }
  }

  bool default_value = (conjunction_type_ == Type::AND);
  cell.set_boolean(default_value);
  return rc;
}
