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
// Created by Wangyunlai on 2023/7/12.
//

#include "sql/executor/load_data_executor.h"
#include "common/lang/string.h"
#include "event/session_event.h"
#include "event/sql_event.h"
#include "sql/executor/sql_result.h"
#include "sql/stmt/load_data_stmt.h"

using namespace common;

RC LoadDataExecutor::execute(SQLStageEvent *sql_event)
{
  RC            rc         = RC::SUCCESS;
  SqlResult    *sql_result = sql_event->session_event()->sql_result();
  LoadDataStmt *stmt       = static_cast<LoadDataStmt *>(sql_event->stmt());
  Table        *table      = stmt->table();
  const char   *file_name  = stmt->filename();
  load_data(table, file_name, sql_result);
  return rc;
}

/**
 * 从文件中导入数据时使用。尝试向表中插入解析后的一行数据。
 * @param table  要导入的表
 * @param file_values 从文件中读取到的一行数据，使用分隔符拆分后的几个字段值
 * @param record_values Table::insert_record使用的参数，为了防止频繁的申请内存
 * @param errmsg 如果出现错误，通过这个参数返回错误信息
 * @return 成功返回RC::SUCCESS
 */
RC insert_record_from_file(
    Table *table, std::vector<std::string> &file_values, std::vector<Value> &record_values, std::stringstream &errmsg)
{

  const int field_num     = record_values.size();
  const int sys_field_num = table->table_meta().sys_field_num();

  if (file_values.size() < record_values.size()) {
    return RC::SCHEMA_FIELD_MISSING;
  }

  RC rc = RC::SUCCESS;

  std::stringstream deserialize_stream;
  for (int i = 0; i < field_num && RC::SUCCESS == rc; i++) {
    const FieldMeta *field = table->table_meta().field(i + sys_field_num);

    std::string &file_value = file_values[i];
    if (field->type() != AttrType::CHARS) {
      common::strip(file_value);
    }
    rc = DataType::type_instance(field->type())->set_value_from_str(record_values[i], file_value);
  }

  if (RC::SUCCESS == rc) {
    Record record;
    rc = table->make_record(field_num, record_values.data(), record);
    if (rc != RC::SUCCESS) {
      errmsg << "insert failed.";
    } else if (RC::SUCCESS != (rc = table->insert_record(record))) {
      errmsg << "insert failed.";
    }
  }
  return rc;
}

void LoadDataExecutor::load_data(Table *table, const char *file_name, SqlResult *sql_result)
{
  std::stringstream result_string;

  std::fstream fs;
  fs.open(file_name, std::ios_base::in | std::ios_base::binary);
  if (!fs.is_open()) {
    result_string << "Failed to open file: " << file_name << ". system error=" << strerror(errno) << std::endl;
    sql_result->set_return_code(RC::FILE_NOT_EXIST);
    sql_result->set_state_string(result_string.str());
    return;
  }

  struct timespec begin_time;
  clock_gettime(CLOCK_MONOTONIC, &begin_time);
  const int sys_field_num = table->table_meta().sys_field_num();
  const int field_num     = table->table_meta().field_num() - sys_field_num;

  std::vector<Value>       record_values(field_num);
  std::string              line;
  std::vector<std::string> file_values;
  const std::string        delim("|");
  int                      line_num        = 0;
  int                      insertion_count = 0;
  RC                       rc              = RC::SUCCESS;
  while (!fs.eof() && RC::SUCCESS == rc) {
    std::getline(fs, line);
    line_num++;
    if (common::is_blank(line.c_str())) {
      continue;
    }

    file_values.clear();
    common::split_string(line, delim, file_values);
    std::stringstream errmsg;
    rc = insert_record_from_file(table, file_values, record_values, errmsg);
    if (rc != RC::SUCCESS) {
      result_string << "Line:" << line_num << " insert record failed:" << errmsg.str() << ". error:" << strrc(rc)
                    << std::endl;
    } else {
      insertion_count++;
    }
  }
  fs.close();

  struct timespec end_time;
  clock_gettime(CLOCK_MONOTONIC, &end_time);
  long cost_nano = (end_time.tv_sec - begin_time.tv_sec) * 1000000000L + (end_time.tv_nsec - begin_time.tv_nsec);
  if (RC::SUCCESS == rc) {
    result_string << strrc(rc) << ". total " << line_num << " line(s) handled and " << insertion_count
                  << " record(s) loaded, total cost " << cost_nano / 1000000000.0 << " second(s)" << std::endl;
  }
  sql_result->set_return_code(RC::SUCCESS);
  sql_result->set_state_string(result_string.str());
}
