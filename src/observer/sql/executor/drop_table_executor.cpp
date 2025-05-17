#include "drop_table_executor.h"

#include "session/session.h"
#include "common/log/log.h"
#include "sql/stmt/drop_table_stmt.h"
#include "event/sql_event.h"
#include "event/session_event.h"
#include "storage/db/db.h"

RC DropTableExecutor::execute(SQLStageEvent *sql_event) {
  Stmt *stmt = sql_event->stmt();
  ASSERT(stmt->type() == StmtType::DROP_TABLE,
         "drop table executor can not run this command: %d", static_cast<int>(stmt->type()));

  DropTableStmt *drop_stmt = static_cast<DropTableStmt *>(stmt);
  Session *session = sql_event->session_event()->session();
  Db *db = session->get_current_db();

  RC rc = db->drop_table(drop_stmt->table_name().c_str());
  return rc;
}
