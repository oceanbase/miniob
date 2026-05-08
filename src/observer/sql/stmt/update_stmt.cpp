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
#include "storage/db/db.h"
#include "storage/table/table.h"
#include "common/type/attr_type.h"
#include "common/type/data_type.h"

UpdateStmt::UpdateStmt(
    Table *table, const char *attribute_name, const Value &value, std::vector<ConditionSqlNode> &&conditions)
    : table_(table), attribute_name_(attribute_name), value_(value), conditions_(std::move(conditions))
{}

RC UpdateStmt::create(Db *db, const UpdateSqlNode &update, Stmt *&stmt)
{
  if (nullptr == db || update.relation_name.empty()) {
    LOG_WARN("invalid argument. db=%p, table_name is empty", db);
    return RC::INVALID_ARGUMENT;
  }

  // 查找表
  const char *table_name = update.relation_name.c_str();
  Table      *table      = db->find_table(table_name);
  if (nullptr == table) {
    LOG_WARN("no such table. db=%s, table_name=%s", db->name(), table_name);
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }

  // 检查字段是否存在
  const FieldMeta *field_meta = table->table_meta().field(update.attribute_name.c_str());
  if (nullptr == field_meta) {
    LOG_WARN("no such field. table=%s, field=%s", table_name, update.attribute_name.c_str());
    return RC::SCHEMA_FIELD_MISSING;
  }

  // 检查值类型是否与字段类型匹配，或可以进行类型转换
  const Value &update_value = update.value;
  AttrType     field_type   = field_meta->type();
  AttrType     value_type   = update_value.attr_type();

  // 对于 TEXT 类型和 CHAR 类型，可以互相转换
  if (field_type != value_type) {
    // 尝试类型转换
    Value converted_value;
    RC    rc = DataType::type_instance(value_type)->cast_to(update_value, field_type, converted_value);
    if (rc != RC::SUCCESS) {
      LOG_WARN("cannot cast value to field type. table=%s, field=%s, field_type=%d, value_type=%d",
               table_name, update.attribute_name.c_str(), field_type, value_type);
      return RC::SCHEMA_FIELD_TYPE_MISMATCH;
    }

    // 使用转换后的值
    stmt = new UpdateStmt(
        table, update.attribute_name.c_str(), converted_value, std::vector<ConditionSqlNode>(update.conditions));
  } else {
    // 类型匹配，直接使用
    stmt = new UpdateStmt(
        table, update.attribute_name.c_str(), update_value, std::vector<ConditionSqlNode>(update.conditions));
  }

  return RC::SUCCESS;
}
