/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#pragma once

#include <string>

#include "sql/stmt/stmt.h"
#include "sql/parser/parse_defs.h"   // 里边有 DropTableSqlNode
#include "storage/db/db.h"

/**
 * DROP TABLE 语句对应的 Stmt
 */
class DropTableStmt : public Stmt
{
public:
  DropTableStmt(Db *db, const char *table_name)
    : db_(db), table_name_(table_name)
  {}

  StmtType type() const override { return StmtType::DROP_TABLE; }

  Db *db() const { return db_; }

  const std::string &table_name() const { return table_name_; }

  // 由 resolve 阶段调用，创建 Stmt
  static RC create(Db *db, const DropTableSqlNode &sql_node, Stmt *&stmt);

private:
  Db          *db_{nullptr};
  std::string  table_name_;
};
