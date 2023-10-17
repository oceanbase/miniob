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
// Created by Wangyunlai on 2023/7/12.
//

#pragma once

#include <string>
#include <vector>

#include "sql/stmt/stmt.h"

class Table;

class LoadDataStmt : public Stmt
{
public:
  LoadDataStmt(Table *table, const char *filename) : table_(table), filename_(filename)
  {}
  virtual ~LoadDataStmt() = default;

  StmtType type() const override { return StmtType::LOAD_DATA; }

  Table *table() const { return table_; }
  const char *filename() const { return filename_.c_str(); }

  static RC create(Db *db, const LoadDataSqlNode &load_data, Stmt *&stmt);

private:
  Table *table_ = nullptr;
  std::string filename_;
};
