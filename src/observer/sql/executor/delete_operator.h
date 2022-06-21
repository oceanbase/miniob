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
// Created by WangYunlai on 2021/6/9.
//

#pragma once

#include "sql/parser/parse.h"
#include "rc.h"

class DeleteStmt;

class DeleteOperator : public Operator
{
public:
  DeleteOperator(DeleteStmt *delete_stmt)
    : delete_stmt_(delete_stmt)
  {}

  virtual ~InsertOperator() = default;

  RC open() override;
  RC next() override;
  RC close() override;

private:
  InsertStmt *insert_stmt_ = nullptr;
};
