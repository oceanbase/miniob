/* Copyright (c) 2021OceanBase and/or its affiliates. All rights reserved.
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

#include "sql/stmt/insert_stmt.h"
#include "common/log/log.h"
#include "storage/db/db.h"
#include "storage/table/table.h"

// bool isValidDateDetailed(const char* strDate) {
//     int year = 0, month = 0, day = 0;
//     sscanf(strDate, "%d-%d-%d", &year, &month, &day);

//     if (year < 1 || month < 1 || month > 12 || day < 1) {
//         return false;
//     }

//     // 每月天数，考虑平年和闰年的2月份
//     int daysInMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
//     if ((year%4==0&&year%100!=0)||(year %400==0)) {
//         daysInMonth[1] = 29; // 闰年2月29天
//     }

//     return day <= daysInMonth[month - 1];
// }

bool isValidDateDetailed(int intDate) {
    // 提取年、月、日
    int year = intDate / 10000;
    int month = (intDate / 100) % 100;
    int day = intDate % 100;

    // 检查年月日的范围
    if (year < 1 || month < 1 || month > 12 || day < 1) {
        return false;
    }

    // 每月天数，考虑平年和闰年的2月份
    int daysInMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
        daysInMonth[1] = 29; // 闰年2月29天
    }
    return day <= daysInMonth[month - 1];
}


InsertStmt::InsertStmt(Table *table, const Value *values, int value_amount)
    : table_(table), values_(values), value_amount_(value_amount)
{}

RC InsertStmt::create(Db *db, const InsertSqlNode &inserts, Stmt *&stmt)
{
  const char *table_name = inserts.relation_name.c_str();
  if (nullptr == db || nullptr == table_name || inserts.values.empty()) {
    LOG_WARN("invalid argument. db=%p, table_name=%p, value_num=%d",
        db, table_name, static_cast<int>(inserts.values.size()));
    return RC::INVALID_ARGUMENT;
  }

  // check whether the table exists
  Table *table = db->find_table(table_name);
  if (nullptr == table) {
    LOG_WARN("no such table. db=%s, table_name=%s", db->name(), table_name);
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }

  // check the fields number
  const Value *values = inserts.values.data();
  const int value_num = static_cast<int>(inserts.values.size());
  const TableMeta &table_meta = table->table_meta();
  const int field_num = table_meta.field_num() - table_meta.sys_field_num();
  if (field_num != value_num) {
    LOG_WARN("schema mismatch. value num=%d, field num in schema=%d", value_num, field_num);
    return RC::SCHEMA_FIELD_MISSING;
  }

  // check fields type
  const int sys_field_num = table_meta.sys_field_num();
  for (int i = 0; i < value_num; i++) {
    const FieldMeta *field_meta = table_meta.field(i + sys_field_num);
    const AttrType field_type = field_meta->type();
    const AttrType value_type = values[i].attr_type();
    if (field_type != value_type) {  // TODO try to convert the value type to field type
      LOG_WARN("field type mismatch. table=%s, field=%s, field type=%d, value_type=%d",
          table_name, field_meta->name(), field_type, value_type);
      return RC::SCHEMA_FIELD_TYPE_MISMATCH;
    }
    if (value_type == DATES && !isValidDateDetailed(values[i].num_value_.date_value_)) {
      LOG_WARN("Invalid date value for field %s: %s", field_meta->name(), values[i].data());
      return RC::INVALID_ARGUMENT;
    }
    // if (field_type == DATES && !isValidDateDetailed(values[i].data())) {
    // LOG_WARN("Invalid date value for field %s: %s", field_meta->name(), values[i].data());
    // return RC::INVALID_ARGUMENT;
    // }
  }

  stmt = new InsertStmt(table, values, value_num);
  return RC::SUCCESS;
}
