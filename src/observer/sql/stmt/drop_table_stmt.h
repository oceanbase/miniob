#pragma once

#include "common/lang/string.h"
#include "sql/stmt/stmt.h"

class Db;

class DropTableStmt : public Stmt
{
public:
  DropTableStmt(const string &table_name) : table_name_(table_name) {}
  virtual ~DropTableStmt() = default;

  StmtType type() const override { return StmtType::DROP_TABLE; }
  const string &table_name() const { return table_name_; }

  static RC create(Db *db, const DropTableSqlNode &drop_table, Stmt *&stmt);

private:
  string table_name_;
};