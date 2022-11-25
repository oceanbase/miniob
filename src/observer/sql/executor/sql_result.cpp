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
// Created by WangYunlai on 2022/11/18.
//

#include "rc.h"
#include "sql/executor/sql_result.h"

void SqlResult::set_tuple_schema(const TupleSchema &schema)
{
  tuple_schema_ = schema;
}

RC SqlResult::open()
{
  if (nullptr == operator_) {
    return RC::INVALID_ARGUMENT;
  }
  return operator_->open();
}

RC SqlResult::close()
{
  if (nullptr == operator_) {
    return RC::INVALID_ARGUMENT;
  }
  return operator_->close();
}

RC SqlResult::next_tuple(Tuple *&tuple)
{
  RC rc = operator_->next();
  if (rc != RC::SUCCESS) {
    return rc;
  }

  tuple = operator_->current_tuple();
  return rc;
}
