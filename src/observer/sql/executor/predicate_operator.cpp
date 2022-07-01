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
// Created by WangYunlai on 2022/6/27.
//

#pragma once

#include "common/log/log.h"
#include "sql/executor/predicate_operator.h"
#include "storage/common/record.h"
#include "sql/stmt/filter_stmt.h"
#include "storage/common/field.h"

RC PredicateOperator::open()
{
  if (children_.size() != 1) {
    LOG_WARN("predicate operator must has one child");
    return RC::INTERNAL;
  }

  return children_[0]->open();
}

RC PredicateOperator::next()
{
  RC rc = RC::SUCCESS;
  Operator *oper = children_[0];
  
  while (RC::SUCCESS == (rc = oper->next())) {
    Tuple *tuple = oper->current_tuple();
    if (nullptr == tuple) {
      rc = RC::INTERNAL;
      LOG_WARN("failed to get tuple from operator");
      break;
    }

    if (do_predicate(static_cast<RowTuple &>(*tuple))) {
      return rc;
    }
  }
  return rc;
}

RC PredicateOperator::close()
{
  children_[0]->close();
  return RC::SUCCESS;
}

Tuple * PredicateOperator::current_tuple()
{
  return children_[0]->current_tuple();
}

void get_cell(const RowTuple &tuple, const FilterItem &filter_item, TupleCell &cell)
{
  if (filter_item.is_attr()) {
    cell.set_data(tuple.record().data() + filter_item.field().field()->offset());
    cell.set_type(filter_item.field().field()->type());
  } else {
    cell.set_data((char *)filter_item.value().data);
    cell.set_type(filter_item.value().type);
  }
}

bool PredicateOperator::do_predicate(RowTuple &tuple)
{
  if (filter_stmt_ == nullptr) {
    return true;
  }

  for (const FilterUnit &filter_unit : filter_stmt_->filter_units()) {
    const FilterItem & left = filter_unit.left();
    const FilterItem & right = filter_unit.right();
    CompOp comp = filter_unit.comp();
    TupleCell left_cell;
    TupleCell right_cell;
    get_cell(tuple, left, left_cell);
    get_cell(tuple, right, right_cell);

    const int compare = left_cell.compare(right_cell);
    switch (comp) {
    case EQUAL_TO: return 0 == compare;
    case LESS_EQUAL: return compare <= 0;
    case NOT_EQUAL: return compare != 0;
    case LESS_THAN: return compare < 0;
    case GREAT_EQUAL: return compare >= 0;
    case GREAT_THAN: return compare > 0;
    default: {
      LOG_WARN("invalid compare type: %d", comp);
    }
    }
  }
  return false;
}
