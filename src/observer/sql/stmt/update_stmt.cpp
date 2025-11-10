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
#include "common/log/log.h"
#include "common/lang/unordered_map.h"
#include "sql/stmt/filter_stmt.h"
#include "storage/db/db.h"
#include "storage/table/table.h"
#include "storage/table/table_meta.h"

UpdateStmt::~UpdateStmt()
{
  if (filter_stmt_ != nullptr) {
    delete filter_stmt_;
    filter_stmt_ = nullptr;
  }
}

RC UpdateStmt::create(Db *db, const UpdateSqlNode &update, Stmt *&stmt)
{
  stmt = nullptr;

  const char *table_name = update.relation_name.c_str();
  if (nullptr == db || nullptr == table_name) {
    LOG_WARN("invalid argument. db=%p, table_name=%p", db, table_name);
    return RC::INVALID_ARGUMENT;
  }

  // find table
  Table *table = db->find_table(table_name);
  if (nullptr == table) {
    LOG_WARN("no such table. db=%s, table_name=%s", db->name(), table_name);
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }

  // find field
  const TableMeta &table_meta = table->table_meta();
  const FieldMeta *field_meta = table_meta.field(update.attribute_name.c_str());
  if (nullptr == field_meta) {
    LOG_WARN("field not exist. table=%s, field=%s", table_name, update.attribute_name.c_str());
    return RC::SCHEMA_FIELD_MISSING;
  }

  // cast value if needed
  Value final_value = update.value;
  if (final_value.attr_type() != field_meta->type()) {
    Value casted;
    RC rc_cast = Value::cast_to(final_value, field_meta->type(), casted);
    if (rc_cast != RC::SUCCESS) {
      LOG_WARN("failed to cast update value. from=%d to=%d", final_value.attr_type(), field_meta->type());
      return rc_cast;
    }
    final_value = std::move(casted);
  }

  // build filter
  unordered_map<string, Table *> table_map;
  table_map.insert({update.relation_name, table});

  FilterStmt *filter_stmt = nullptr;
  RC rc = FilterStmt::create(db,
                             table,
                             &table_map,
                             update.conditions.data(),
                             static_cast<int>(update.conditions.size()),
                             filter_stmt);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to create filter statement. rc=%s", strrc(rc));
    return rc;
  }

  stmt = new UpdateStmt(table, field_meta, final_value, filter_stmt);
  return RC::SUCCESS;
}
