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
// Created by Wangyunlai on 2023/6/14.
//

#pragma once

#include <string>
#include <vector>

#include "sql/stmt/stmt.h"

/**
 * @brief 事务的 Commit/Rollback 语句，现在什么成员都没有
 * @ingroup Statement
 */
class TrxEndStmt : public Stmt
{
public:
  TrxEndStmt(StmtType type) : type_(type) {}
  virtual ~TrxEndStmt() = default;

  StmtType type() const override { return type_; }

  static RC create(SqlCommandFlag flag, Stmt *&stmt)
  {
    StmtType type = flag == SqlCommandFlag::SCF_COMMIT ? StmtType::COMMIT : StmtType::ROLLBACK;
    stmt          = new TrxEndStmt(type);
    return RC::SUCCESS;
  }

private:
  StmtType type_;
};