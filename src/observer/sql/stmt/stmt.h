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

#include "common/rc.h"
#include "sql/parser/parse_defs.h"

class Db;

/**
 * @brief Statement SQL语句解析后通过Resolver转换成Stmt
 * @defgroup Statement
 * @file stmt.h
 */

/**
 * @brief Statement的类型
 * 
 */
#define DEFINE_ENUM()               \
  DEFINE_ENUM_ITEM(CALC)            \
  DEFINE_ENUM_ITEM(SELECT)          \
  DEFINE_ENUM_ITEM(INSERT)          \
  DEFINE_ENUM_ITEM(UPDATE)          \
  DEFINE_ENUM_ITEM(DELETE)          \
  DEFINE_ENUM_ITEM(CREATE_TABLE)    \
  DEFINE_ENUM_ITEM(DROP_TABLE)      \
  DEFINE_ENUM_ITEM(CREATE_INDEX)    \
  DEFINE_ENUM_ITEM(DROP_INDEX)      \
  DEFINE_ENUM_ITEM(SYNC)            \
  DEFINE_ENUM_ITEM(SHOW_TABLES)     \
  DEFINE_ENUM_ITEM(DESC_TABLE)      \
  DEFINE_ENUM_ITEM(BEGIN)           \
  DEFINE_ENUM_ITEM(COMMIT)          \
  DEFINE_ENUM_ITEM(ROLLBACK)        \
  DEFINE_ENUM_ITEM(LOAD_DATA)       \
  DEFINE_ENUM_ITEM(HELP)            \
  DEFINE_ENUM_ITEM(EXIT)            \
  DEFINE_ENUM_ITEM(EXPLAIN)         \
  DEFINE_ENUM_ITEM(PREDICATE)       \
  DEFINE_ENUM_ITEM(SET_VARIABLE)

enum class StmtType {
  #define DEFINE_ENUM_ITEM(name)  name,
  DEFINE_ENUM()
  #undef DEFINE_ENUM_ITEM
};

inline const char *stmt_type_name(StmtType type)
{
  switch (type) {
    #define DEFINE_ENUM_ITEM(name)  case StmtType::name: return #name;
    DEFINE_ENUM()
    #undef DEFINE_ENUM_ITEM
    default: return "unkown";
  }
}

/**
 * @brief Stmt for Statement
 * @ingroup Statement
 * @details SQL解析后的语句，再进一步解析成Stmt，使用内部的数据结构来表示。
 * 比如table_name，解析成具体的 Table对象，attr/field name解析成Field对象。
 */
class Stmt
{
public:
  Stmt() = default;
  virtual ~Stmt() = default;

  virtual StmtType type() const = 0;

public:
  static RC create_stmt(Db *db, ParsedSqlNode &sql_node, Stmt *&stmt);

private:
};
