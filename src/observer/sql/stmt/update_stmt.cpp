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
#include "sql/stmt/filter_stmt.h"
#include "storage/db/db.h"
#include "storage/table/table.h"

UpdateStmt::UpdateStmt(Table *table,Field field , Value value, FilterStmt *filter_stmt)
    : table_(table), value_(value), field_(field), filter_stmt_(filter_stmt)
{}

RC UpdateStmt::create(Db *db, const UpdateSqlNode &update_sql, Stmt *&stmt)
{
  const char *table_name=update_sql.relation_name.c_str();

  if (nullptr == db || nullptr == table_name) {
    LOG_WARN("invalid argument. db=%p, table_name=%p", db, table_name);
    return RC::INVALID_ARGUMENT;
  }

  Table *table = db->find_table(table_name);
  if (nullptr == table) {
    LOG_WARN("no such table. db=%s, table_name=%s", db->name(), table_name);
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }

  const FieldMeta *field_meta=table->table_meta().field(update_sql.attribute_name.c_str());  
  if (nullptr==field_meta){
    LOG_WARN("no such field. field=%s.%s.%s",db->name(),table->name(),update_sql.attribute_name);
    return RC::SCHEMA_FIELD_MISSING;
  }


  std::unordered_map<std::string, Table *> table_map;
  table_map.insert(std::pair<std::string, Table *>(std::string(table_name), table));
  
  FilterStmt *filter_stmt = nullptr;
  RC          rc          = FilterStmt::create(
      db, table, &table_map, update_sql.conditions.data(), static_cast<int>(update_sql.conditions.size()), filter_stmt);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to create filter statement. rc=%d:%s", rc, strrc(rc));
    return rc;
  }
////////////////////////////////////////////////////
  Field s_field = Field(table, field_meta);

//insert
  Value value     = update_sql.value;

  // check fields type
  const AttrType   value_type = value.attr_type();
  const AttrType   field_type = field_meta->type();
  if (field_type != value_type) {  // TODO try to convert the value type to field type
      LOG_WARN("field type mismatch. table=%s, field=%s, field type=%d, value_type=%d",
          table_name, field_meta->name(), field_type, value_type);
      return RC::SCHEMA_FIELD_TYPE_MISMATCH;
    }
  // TODO
  stmt = new UpdateStmt(table, s_field ,value, filter_stmt);
  return rc;
}
