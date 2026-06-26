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
// Created by Wangyunlai on 2022/5/22.
//

#pragma once

#include "common/sys/rc.h"
#include "sql/stmt/stmt.h"
#include "sql/parser/parse_defs.h"
#include <vector>

class Table;

/**
 * @brief 更新语句
 * @ingroup Statement
 */
class UpdateStmt : public Stmt
{
public:
  UpdateStmt() = default;
  UpdateStmt(Table *table, const char *attribute_name, const Value &value, std::vector<ConditionSqlNode> &&conditions);

  StmtType type() const override { return StmtType::UPDATE; }

public:
  static RC create(Db *db, const UpdateSqlNode &update_sql, Stmt *&stmt);

public:
  Table                               *table() const { return table_; }
  const char                          *attribute_name() const { return attribute_name_.c_str(); }
  const Value                         &value() const { return value_; }
  const std::vector<ConditionSqlNode> &conditions() const { return conditions_; }

private:
  Table                        *table_ = nullptr;
  std::string                   attribute_name_;
  Value                         value_;
  std::vector<ConditionSqlNode> conditions_;
};
