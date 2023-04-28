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
// Created by Wangyunlai on 2023/04/24.
//

#include "storage/common/field.h"
#include "sql/expr/tuple_cell.h"
#include "storage/record/record.h"

void Field::set_int(Record &record, int value)
{
  TupleCell cell(field_, record.data() + field_->offset(), field_->len());
  cell.set_int(value);
}

int Field::get_int(const Record &record)
{
  TupleCell cell(field_, const_cast<char *>(record.data() + field_->offset()), field_->len());
  return cell.get_int();
}
