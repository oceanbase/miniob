#include "sql/stmt/drop_table_stmt.h"
#include "storage/db/db.h"

RC DropTableStmt::create(Db *db, const DropTableSqlNode &drop_table, Stmt *&stmt)
{
  // 当前只保存表名，不做额外的存在性检查
  // 存在性检查可以在执行阶段做
  stmt = new DropTableStmt(drop_table.relation_name);
  return RC::SUCCESS;
}