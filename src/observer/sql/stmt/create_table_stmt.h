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
// Created by Wangyunlai on 2023/6/13.
//

#pragma once

#include "common/lang/string.h"
#include "common/lang/vector.h"
#include "sql/stmt/stmt.h"

class Db;

/**
 * @brief 表示创建表的语句
 * @ingroup Statement
 * @details 虽然解析成了stmt，但是与原始的SQL解析后的数据也差不多
 */
class CreateTableStmt : public Stmt
{
public:
  CreateTableStmt(const string &table_name, const vector<AttrInfoSqlNode> &attr_infos, const vector<string> &pks,
      StorageFormat storage_format)
      : table_name_(table_name), attr_infos_(attr_infos), primary_keys_(pks), storage_format_(storage_format)
  {}
  virtual ~CreateTableStmt() = default;

  StmtType type() const override { return StmtType::CREATE_TABLE; }

  const string                  &table_name() const { return table_name_; }
  const vector<AttrInfoSqlNode> &attr_infos() const { return attr_infos_; }
  const vector<string>          &primary_keys() const { return primary_keys_; }
  const StorageFormat            storage_format() const { return storage_format_; }

  static RC            create(Db *db, const CreateTableSqlNode &create_table, Stmt *&stmt);
  static StorageFormat get_storage_format(const char *format_str);

private:
  string                  table_name_;
  vector<AttrInfoSqlNode> attr_infos_;
  vector<string>          primary_keys_;
  StorageFormat           storage_format_;
};
