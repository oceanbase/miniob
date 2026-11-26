/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
*/
#pragma once

#include "common/sys/rc.h"
#include "common/value.h"
#include "sql/stmt/stmt.h"

class Table;
class FilterStmt;

class UpdateStmt : public Stmt
{
public:
  UpdateStmt(Table *table, const char *attribute_name, const Value &value, FilterStmt *filter_stmt);
  ~UpdateStmt() override;

  StmtType type() const override { return StmtType::UPDATE; }
  Table *table() const { return table_; }
  const char *attribute_name() const { return attribute_name_.c_str(); }
  const Value &value() const { return value_; }
  FilterStmt *filter_stmt() const { return filter_stmt_; }

  static RC create(Db *db, const UpdateSqlNode &update_sql, Stmt *&stmt);

private:
  Table *table_ = nullptr;
  string attribute_name_;
  Value value_;
  FilterStmt *filter_stmt_ = nullptr;
};
