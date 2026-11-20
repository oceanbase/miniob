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

#include <cstring>

#include "common/log/log.h"
#include "sql/stmt/update_stmt.h"
#include "storage/common/db.h"
#include "storage/common/table.h"
#include "storage/common/table_meta.h"

UpdateStmt::UpdateStmt(Table *table, const char *attribute_name, const Value &value, int condition_num,
    const Condition conditions[])
  : table_(table), attribute_name_(attribute_name == nullptr ? "" : attribute_name), condition_num_(condition_num)
{
  // 解析树在语句执行结束后会被释放，这里把更新值拷贝一份
  size_t value_len = 4;  // int / float
  if (value.type == CHARS) {
    value_len = strlen(static_cast<const char *>(value.data)) + 1;
  }
  value_buffer_.assign(
      static_cast<const char *>(value.data), static_cast<const char *>(value.data) + value_len);
  value_.type = value.type;
  value_.data = value_buffer_.data();

  if (condition_num_ > MAX_NUM) {
    condition_num_ = MAX_NUM;
  }
  for (int i = 0; i < condition_num_; i++) {
    conditions_[i] = conditions[i];
  }
}

RC UpdateStmt::create(Db *db, const Updates &update, Stmt *&stmt)
{
  stmt = nullptr;

  const char *table_name = update.relation_name;
  const char *attribute_name = update.attribute_name;
  if (db == nullptr || table_name == nullptr || attribute_name == nullptr) {
    LOG_WARN("invalid argument. db=%p, table_name=%p, attribute_name=%p", db, table_name, attribute_name);
    return RC::INVALID_ARGUMENT;
  }

  Table *table = db->find_table(table_name);
  if (table == nullptr) {
    LOG_WARN("no such table. db=%s, table_name=%s", db->name(), table_name);
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }

  const FieldMeta *field = table->table_meta().field(attribute_name);
  if (field == nullptr) {
    LOG_WARN("no such field. table=%s, field=%s", table_name, attribute_name);
    return RC::SCHEMA_FIELD_NOT_EXIST;
  }

  if (field->type() != update.value.type) {
    LOG_WARN("field type mismatch. table=%s, field=%s, field_type=%d, value_type=%d", table_name, attribute_name,
        field->type(), update.value.type);
    return RC::SCHEMA_FIELD_TYPE_MISMATCH;
  }

  stmt = new UpdateStmt(table, attribute_name, update.value, static_cast<int>(update.condition_num), update.conditions);
  return RC::SUCCESS;
}
