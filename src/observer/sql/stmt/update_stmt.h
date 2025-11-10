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
#include "common/value.h"

class Table;
class FilterStmt;
class FieldMeta;

/**
 * @brief 更新语句
 * @ingroup Statement
 */
class UpdateStmt : public Stmt
{
public:
  UpdateStmt()  = default;
  UpdateStmt(Table *table, const FieldMeta *field, const Value &value, FilterStmt *filter_stmt)
      : table_(table), field_(field), value_(value), filter_stmt_(filter_stmt)
  {}
  ~UpdateStmt();

public:
  static RC create(Db *db, const UpdateSqlNode &update_sql, Stmt *&stmt);

public:
  StmtType type() const override { return StmtType::UPDATE; }

  Table *table() const { return table_; }
  const FieldMeta *field_meta() const { return field_; }
  const Value &value() const { return value_; }
  FilterStmt *filter_stmt() const { return filter_stmt_; }

private:
  Table *table_            = nullptr;
  const FieldMeta *field_  = nullptr;
  Value value_;
  FilterStmt *filter_stmt_ = nullptr;
};
