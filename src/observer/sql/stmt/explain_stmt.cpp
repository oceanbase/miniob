/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Wangyunlai on 2022/12/27.
//

#include "sql/stmt/explain_stmt.h"
#include "sql/stmt/stmt.h"
#include "common/log/log.h"

ExplainStmt::ExplainStmt(std::unique_ptr<Stmt> child_stmt) : child_stmt_(std::move(child_stmt))
{}

RC ExplainStmt::create(Db *db, const ExplainSqlNode &explain, Stmt *&stmt)
{
  Stmt *child_stmt = nullptr;
  RC rc = Stmt::create_stmt(db, *explain.sql_node, child_stmt);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to create explain's child stmt. rc=%s", strrc(rc));
    return rc;
  }

  std::unique_ptr<Stmt> child_stmt_ptr = std::unique_ptr<Stmt>(child_stmt);
  stmt = new ExplainStmt(std::move(child_stmt_ptr));
  return rc;
}
