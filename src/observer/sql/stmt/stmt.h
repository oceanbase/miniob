/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
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

#include "rc.h"
#include "sql/parser/parse_defs.h"

class Db;

enum class StmtType
{
  SELECT,
  INSERT,
  UPDATE,
  DELETE,
  CREATE_TABLE,
  DROP_TABLE,
  CREATE_INDEX,
  DROP_INDEX,
  SYNC,
  SHOW_TABLES,
  DESC_TABLE,
  BEGIN,
  COMMIT,
  ROLLBACK,
  LOAD_DATA,
  HELP,
  EXIT,

  PREDICATE,
};

class Stmt 
{
public:

  Stmt() = default;
  virtual ~Stmt() = default;

  virtual StmtType type() const = 0;

public:
  static RC create_stmt(Db *db, const Query &query, Stmt *&stmt);

private:
};

