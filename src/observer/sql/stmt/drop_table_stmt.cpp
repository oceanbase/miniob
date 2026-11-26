#include "sql/stmt/drop_table_stmt.h"

#include "storage/db/db.h"

RC DropTableStmt::create(Db *db, const DropTableSqlNode &sql, Stmt *&stmt)
{
  stmt = nullptr;
  if (db == nullptr || sql.relation_name.empty()) {
    return RC::INVALID_ARGUMENT;
  }
  if (db->find_table(sql.relation_name.c_str()) == nullptr) {
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }
  stmt = new DropTableStmt(sql.relation_name);
  return RC::SUCCESS;
}
