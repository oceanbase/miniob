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
#include "common/lang/string.h"
#include "common/log/log.h"
#include "sql/stmt/filter_stmt.h"
#include "storage/db/db.h"
#include "storage/table/table.h"

UpdateStmt::UpdateStmt(Table *table, const char *attribute_name, const Value *value, FilterStmt *filter_stmt)
    : table_(table), attribute_name_(attribute_name), value_(value), filter_stmt_(filter_stmt)
{}

UpdateStmt::~UpdateStmt()
{
  if (nullptr != filter_stmt_) {
    delete filter_stmt_;
    filter_stmt_ = nullptr;
  }
}

RC UpdateStmt::create(Db *db, const UpdateSqlNode &update, Stmt *&stmt)
{
  // 检查表是否存在
  Table *table = db->find_table(update.relation_name.c_str());
  if (nullptr == table) {
    LOG_WARN("no such table. db=%s, table_name=%s", db->name(), update.relation_name.c_str());
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }

  // 检查字段是否存在
  const FieldMeta *field_meta = table->table_meta().field(update.attribute_name.c_str());
  if (nullptr == field_meta) {
    LOG_WARN("no such field in table. db=%s, table=%s, field=%s", 
             db->name(), table->name(), update.attribute_name.c_str());
    return RC::SCHEMA_FIELD_NOT_EXIST;
  }

  // 检查值类型是否匹配
  AttrType field_type = field_meta->type();
  AttrType value_type = update.value.attr_type();
  
  if (field_type != value_type) {
    // 在某些情况下，我们可以进行类型转换，这里简化处理
    if (!((field_type == AttrType::INTS && value_type == AttrType::FLOATS) || 
          (field_type == AttrType::FLOATS && value_type == AttrType::INTS))) {
      LOG_WARN("field type mismatch. field type=%d, value type=%d", field_type, value_type);
      return RC::SCHEMA_FIELD_TYPE_MISMATCH;
    }
  }

  // 处理 WHERE 条件
  FilterStmt *filter_stmt = nullptr;
  RC rc = RC::SUCCESS;
  if (!update.conditions.empty()) {
    unordered_map<string, Table *> table_map;
    table_map.insert(pair<string, Table *>(string(update.relation_name), table));
    
    rc = FilterStmt::create(db, table, &table_map, 
                           update.conditions.data(), 
                           static_cast<int>(update.conditions.size()), 
                           filter_stmt);
    if (rc != RC::SUCCESS) {
      LOG_WARN("cannot construct filter stmt");
      return rc;
    }
  }

  // 创建 UpdateStmt
  stmt = new UpdateStmt(table, update.attribute_name.c_str(), &update.value, filter_stmt);
  return RC::SUCCESS;
}
  