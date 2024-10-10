// zzw
#pragma once

#include <string>
#include <vector>

#include "sql/stmt/stmt.h"

class Db;

/**
 * @brief 表示删除表的语句
 * @ingroup Statement
 */
class DropTableStmt : public Stmt
{
public:
  DropTableStmt(
    
  // table_name 要删除的表名
  const std::string &table_name): table_name_(table_name){}
  virtual ~DropTableStmt() = default;

  // 获得此s语句的类型(sql_node.flag 表示SQL语句的类型)
  StmtType type() const override { return StmtType::DROP_TABLE; }

  const std::string& table_name() const { return table_name_; }

  // static不依赖于实体
  // 用于根据语法解析树sql结点 DropTableSqlNode 创建一个 Stmt 对象
  static RC create(Db *db, const DropTableSqlNode &drop_table, Stmt *&stmt);
private:
  std::string                  table_name_; // Stmt 将 table_name 解析为具体对象
};