// 必须包含头文件后再定义析构函数
#include "sql/stmt/update_stmt.h"
#include "sql/stmt/filter_stmt.h"

UpdateStmt::~UpdateStmt() {
  delete[] values_;
  delete filter_stmt_;
}
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


#include "sql/stmt/update_stmt.h"
#include "sql/stmt/filter_stmt.h"
#include "storage/db/db.h"
#include <unordered_map>
StmtType UpdateStmt::type() const {
  return StmtType::UPDATE;
}


UpdateStmt::UpdateStmt(Table *table, const std::string &update_field_name, Value *values, int value_amount, FilterStmt *filter_stmt)
  : table_(table), update_field_name_(update_field_name), values_(values), value_amount_(value_amount), filter_stmt_(filter_stmt) {}

RC UpdateStmt::create(Db *db, const UpdateSqlNode &update, Stmt *&stmt)
{
  stmt = nullptr;
  if (db == nullptr) {
    return RC::INVALID_ARGUMENT;
  }
  // 查找表
  Table *table = db->find_table(update.relation_name.c_str());
  if (table == nullptr) {
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }

  // 构造 filter_stmt（条件）
  unordered_map<string, Table *> table_map;
  table_map.insert(std::make_pair(update.relation_name, table));
  FilterStmt *filter_stmt = nullptr;
  RC rc = RC::SUCCESS;
  if (!update.conditions.empty()) {
    rc = FilterStmt::create(db, table, &table_map, update.conditions.data(), static_cast<int>(update.conditions.size()), filter_stmt);
    if (rc != RC::SUCCESS) {
      return rc;
    }
  }

  // 构造 update 的值
  Value *values = new Value[1];
  values[0] = update.value;
  stmt = new UpdateStmt(table, update.attribute_name, values, 1, filter_stmt);
  return RC::SUCCESS;
}
