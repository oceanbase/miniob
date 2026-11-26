/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
*/
#include "sql/stmt/update_stmt.h"

#include "common/log/log.h"
#include "sql/stmt/filter_stmt.h"
#include "storage/db/db.h"
#include "storage/field/field_meta.h"
#include "storage/table/table.h"

UpdateStmt::UpdateStmt(Table *table, const char *attribute_name, const Value &value, FilterStmt *filter_stmt)
    : table_(table), attribute_name_(attribute_name), value_(value), filter_stmt_(filter_stmt)
{}

UpdateStmt::~UpdateStmt()
{
  delete filter_stmt_;
  filter_stmt_ = nullptr;
}

RC UpdateStmt::create(Db *db, const UpdateSqlNode &update, Stmt *&stmt)
{
  stmt = nullptr;
  if (db == nullptr || update.relation_name.empty() || update.attribute_name.empty()) {
    return RC::INVALID_ARGUMENT;
  }

  Table *table = db->find_table(update.relation_name.c_str());
  if (table == nullptr) {
    LOG_WARN("no such table. db=%s, table_name=%s", db->name(), update.relation_name.c_str());
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }

  const FieldMeta *field = table->table_meta().field(update.attribute_name.c_str());
  if (field == nullptr || !field->visible()) {
    LOG_WARN("no such update field. table=%s, field=%s", table->name(), update.attribute_name.c_str());
    return RC::SCHEMA_FIELD_NOT_EXIST;
  }

  Value converted_value;
  RC rc = update.value.attr_type() == field->type()
              ? (converted_value = update.value, RC::SUCCESS)
              : Value::cast_to(update.value, field->type(), converted_value);
  if (rc != RC::SUCCESS) {
    LOG_WARN("cannot cast update value to field type. table=%s, field=%s", table->name(), field->name());
    return rc;
  }

  unordered_map<string, Table *> table_map;
  table_map.emplace(update.relation_name, table);
  FilterStmt *filter_stmt = nullptr;
  rc = FilterStmt::create(db, table, &table_map, update.conditions.data(),
      static_cast<int>(update.conditions.size()), filter_stmt);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to create update filter. rc=%s", strrc(rc));
    return rc;
  }

  stmt = new UpdateStmt(table, update.attribute_name.c_str(), converted_value, filter_stmt);
  return RC::SUCCESS;
}
