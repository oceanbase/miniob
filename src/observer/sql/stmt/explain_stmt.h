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

#pragma once

#include <memory>
#include "sql/stmt/stmt.h"

/**
 * @brief explain语句
 * @ingroup Statement
 */
class ExplainStmt : public Stmt 
{
public:
  ExplainStmt(std::unique_ptr<Stmt> child_stmt);
  virtual ~ExplainStmt() = default;

  StmtType type() const override
  {
    return StmtType::EXPLAIN;
  }

  Stmt *child() const
  {
    return child_stmt_.get();
  }

  static RC create(Db *db, const ExplainSqlNode &query, Stmt *&stmt);

private:
  std::unique_ptr<Stmt> child_stmt_;
};
