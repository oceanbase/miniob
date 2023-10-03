#include "sql/executor/drop_table_executor.h"

#include "session/session.h"
#include "common/log/log.h"
#include "storage/table/table.h"
#include "sql/stmt/drop_table_stmt.h"
#include "event/sql_event.h"
#include "event/session_event.h"
#include "storage/db/db.h"

RC DropTableExecutor::execute(SQLStageEvent *sql_event)
{
  //从 SQLStageEvent 对象中获取 SQL 语句对象，并将其赋值给 stmt 变量
  Stmt *stmt = sql_event->stmt(); 
  //从 SQLStageEvent 对象中获取会话事件对象，然后再从会话事件对象获取会话对象，并将其赋值给 session 变量
  Session *session = sql_event->session_event()->session();
  // 使用断言（ASSERT）检查获取到的 SQL 语句对象的类型是否为 DROP_TABLE，如果不是，则抛出错误。
  ASSERT(stmt->type() == StmtType::DROP_TABLE, 
         "drop table executor can not run this command: %d", static_cast<int>(stmt->type()));
  //将获取到的 SQL 语句对象转换为 DropTableStmt 类型的指针，并将其赋值给 drop_table_stmt 变量
  DropTableStmt *drop_table_stmt = static_cast<DropTableStmt *>(stmt);
  //获取 drop_table_stmt 对象中的表名，并将其转换为 C 风格的字符串（const char* 类型），并将结果赋值给 table_name 变量
  const char *table_name = drop_table_stmt->table_name().c_str();
  //通过会话对象获取当前数据库，并调用当前数据库的 drop_table 方法，传递表名、属性数量和属性信息列表来创建表，并将返回值赋值给 rc 变量
  RC rc = session->get_current_db()->drop_table(table_name);

  return rc;
}