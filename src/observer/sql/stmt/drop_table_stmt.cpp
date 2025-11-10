/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
*/

#include "sql/stmt/drop_table_stmt.h"
#include "common/log/log.h"

RC DropTableStmt::create(Db *db, const DropTableSqlNode &sql_node, Stmt *&stmt)
{
  stmt = nullptr;

  if (db == nullptr) {
    LOG_WARN("invalid argument: db is null");
    return RC::INVALID_ARGUMENT;
  }

  if (sql_node.relation_name.empty()) {
    LOG_WARN("invalid argument: empty table name");
    return RC::INVALID_ARGUMENT;
  }

  stmt = new DropTableStmt(db, sql_node.relation_name.c_str());
  return RC::SUCCESS;
}
