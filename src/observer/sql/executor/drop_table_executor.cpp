// 原有保留
#include "sql/executor/execute_stage.h"
#include "sql/executor/drop_table_executor.h"
#include "sql/stmt/drop_table_stmt.h"
#include "storage/db/db.h"
#include "session/session.h"
#include "common/sys/rc.h"

// ✅ 修正后 真实存在的头文件
#include "event/sql_event.h"       // SQLStageEvent 完整定义就在这里！
#include "event/session_event.h"   // SessionEvent 完整定义
#include "sql/stmt/stmt.h"         // Stmt 基类完整定义

RC DropTableExecutor::execute(SQLStageEvent *sql_event)
{
  // 1. 获取 Stmt 对象
  Stmt *stmt = sql_event->stmt();
  DropTableStmt *drop_stmt = static_cast<DropTableStmt *>(stmt);

  // 2. 获取当前数据库
  Session *session = sql_event->session_event()->session();
  Db *db = session->get_current_db();

  // 3. 调用存储层删除表
  return db->drop_table(drop_stmt->table_name().c_str());
}
