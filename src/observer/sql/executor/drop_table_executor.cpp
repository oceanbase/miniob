/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
*/

#include "sql/executor/drop_table_executor.h"

#include "common/log/log.h"
#include "event/session_event.h"
#include "event/sql_event.h"
#include "sql/stmt/stmt.h"
#include "sql/stmt/drop_table_stmt.h"
#include "sql/executor/sql_result.h"
#include "storage/db/db.h"

RC DropTableExecutor::execute(SQLStageEvent *sql_event)
{
  RC rc = RC::SUCCESS;

  SessionEvent *session_event = sql_event->session_event();
  SqlResult    *sql_result    = session_event->sql_result();
  Stmt         *stmt          = sql_event->stmt();

  if (stmt == nullptr || stmt->type() != StmtType::DROP_TABLE) {
    LOG_WARN("invalid stmt for DropTableExecutor");
    rc = RC::INVALID_ARGUMENT;
    sql_result->set_return_code(rc);
    return rc;
  }

  auto *drop_stmt = static_cast<DropTableStmt *>(stmt);
  Db   *db        = drop_stmt->db();

  if (db == nullptr) {
    LOG_WARN("db is null in DropTableStmt");
    rc = RC::INVALID_ARGUMENT;
    sql_result->set_return_code(rc);
    return rc;
  }

  const std::string &table_name = drop_stmt->table_name();

  rc = db->drop_table(table_name.c_str());
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to drop table %s, rc=%s", table_name.c_str(), strrc(rc));
  }

  sql_result->set_return_code(rc);
  return rc;
}
