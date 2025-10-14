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
// Created by Wangyunlai on 2022/5/22.
//

#pragma once


#include "common/sys/rc.h"
#include "sql/stmt/stmt.h"

#include <string>

class Table;
class FilterStmt;

/**
 * @brief 更新语句
 * @ingroup Statement
 */
class UpdateStmt : public Stmt
{
public:
  ~UpdateStmt();
  StmtType type() const override;
  UpdateStmt() = default;
  UpdateStmt(Table *table, const std::string &update_field_name, Value *values, int value_amount, FilterStmt *filter_stmt = nullptr);

public:
  static RC create(Db *db, const UpdateSqlNode &update_sql, Stmt *&stmt);

public:
  Table *table() const { return table_; }
  const std::string &update_field_name() const { return update_field_name_; }
  Value *values() const { return values_; }
  int    value_amount() const { return value_amount_; }
  FilterStmt *filter_stmt() const { return filter_stmt_; }

private:
  Table *table_        = nullptr;
  std::string update_field_name_;
  Value *values_       = nullptr;
  int    value_amount_ = 0;
  FilterStmt *filter_stmt_ = nullptr;
};
