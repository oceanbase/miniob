/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "sql/executor/analyze_table_executor.h"

#include "common/log/log.h"
#include "event/session_event.h"
#include "event/sql_event.h"
#include "session/session.h"
#include "sql/stmt/analyze_table_stmt.h"
#include "storage/db/db.h"
#include "storage/table/table.h"
#include "catalog/catalog.h"
#include "storage/record/record_scanner.h"

using namespace std;

RC AnalyzeTableExecutor::execute(SQLStageEvent *sql_event)
{
  RC            rc            = RC::SUCCESS;
  Stmt         *stmt          = sql_event->stmt();
  SessionEvent *session_event = sql_event->session_event();
  Session      *session       = session_event->session();
  ASSERT(stmt->type() == StmtType::ANALYZE_TABLE,
      "analyze table executor can not run this command: %d",
      static_cast<int>(stmt->type()));

  AnalyzeTableStmt *analyze_table_stmt = static_cast<AnalyzeTableStmt *>(stmt);
  SqlResult     *sql_result      = session_event->sql_result();
  const char    *table_name      = analyze_table_stmt->table_name().c_str();

  Db    *db    = session->get_current_db();
  Table *table = db->find_table(table_name);
  if (table != nullptr) {
    // TODO: optimize the analyze table compute. we can only get table statistics from metadata
    // Don't really scan the whole table!!
    int table_id = table->table_id();
    table->get_record_scanner(scanner_, session->current_trx(), ReadWriteMode::READ_ONLY);
    Record dummy;
    int row_nums = 0;
    while (OB_SUCC(rc = scanner_->next(dummy))) {
      row_nums++;
    }
    if (rc == RC::RECORD_EOF) {
      rc = RC::SUCCESS;
    } else {
      return rc;
    }
  
    TableStats stats(row_nums);
    Catalog::get_instance().update_table_stats(table_id, stats);
  } else {
    sql_result->set_return_code(RC::SCHEMA_TABLE_NOT_EXIST);
    sql_result->set_state_string("Table not exists");
  }
  return rc;
}

AnalyzeTableExecutor::~AnalyzeTableExecutor() 
{
  if (scanner_ != nullptr) {
    delete scanner_;
    scanner_ = nullptr;
  }
}
