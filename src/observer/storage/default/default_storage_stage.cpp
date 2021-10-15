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
// Created by Longda on 2021/4/13.
//

#include <string.h>
#include <string>

#include "storage/default/default_storage_stage.h"

#include "common/conf/ini.h"
#include "common/io/io.h"
#include "common/lang/string.h"
#include "common/log/log.h"
#include "common/seda/timer_stage.h"
#include "common/metrics/metrics_registry.h"
#include "rc.h"
#include "storage/default/default_handler.h"
#include "storage/common/condition_filter.h"
#include "storage/common/table.h"
#include "storage/common/table_meta.h"
#include "storage/trx/trx.h"
#include "event/execution_plan_event.h"
#include "event/session_event.h"
#include "event/sql_event.h"
#include "event/storage_event.h"
#include "session/session.h"

using namespace common;

const std::string DefaultStorageStage::QUERY_METRIC_TAG = "DefaultStorageStage.query";
const char * CONF_BASE_DIR = "BaseDir";
const char * CONF_SYSTEM_DB = "SystemDb";

const char * DEFAULT_SYSTEM_DB = "sys";

//! Constructor
DefaultStorageStage::DefaultStorageStage(const char *tag) : Stage(tag), handler_(nullptr) {
}

//! Destructor
DefaultStorageStage::~DefaultStorageStage() {
  delete handler_;
  handler_ = nullptr;
}

//! Parse properties, instantiate a stage object
Stage *DefaultStorageStage::make_stage(const std::string &tag) {
  DefaultStorageStage *stage = new (std::nothrow) DefaultStorageStage(tag.c_str());
  if (stage == nullptr) {
    LOG_ERROR("new DefaultStorageStage failed");
    return nullptr;
  }
  stage->set_properties();
  return stage;
}

//! Set properties for this object set in stage specific properties
bool DefaultStorageStage::set_properties() {
  std::string stageNameStr(stage_name_);
  std::map<std::string, std::string> section = 
      get_properties()->get(stageNameStr);
  
  // 初始化时打开默认的database，没有的话会自动创建
  std::map<std::string, std::string>::iterator iter = section.find(CONF_BASE_DIR);
  if (iter == section.end()) {
    LOG_ERROR("Config cannot be empty: %s", CONF_BASE_DIR);
    return false;
  }

  const char *base_dir = iter->second.c_str();
  
  const char *sys_db = DEFAULT_SYSTEM_DB;
  iter = section.find(CONF_SYSTEM_DB);
  if (iter != section.end()) {
    sys_db = iter->second.c_str();
    LOG_INFO("Use %s as system db", sys_db);
  }

  handler_ = &DefaultHandler::get_default();
  if (RC::SUCCESS != handler_->init(base_dir)) {
    LOG_ERROR("Failed to init default handler");
    return false;
  }

  RC ret = handler_->create_db(sys_db);
  if (ret != RC::SUCCESS && ret != RC::SCHEMA_DB_EXIST) {
    LOG_ERROR("Failed to create system db");
    return false;
  }

  ret = handler_->open_db(sys_db);
  if (ret != RC::SUCCESS) {
    LOG_ERROR("Failed to open system db");
    return false;
  }

  Session &default_session = Session::default_session();
  default_session.set_current_db(sys_db);

  LOG_INFO("Open system db success: %s", sys_db);
  return true;
}

//! Initialize stage params and validate outputs
bool DefaultStorageStage::initialize() {
  LOG_TRACE("Enter");

  MetricsRegistry &metricsRegistry = get_metrics_registry();
  query_metric_ =  new SimpleTimer();
  metricsRegistry.register_metric(QUERY_METRIC_TAG, query_metric_);

  LOG_TRACE("Exit");
  return true;
}

//! Cleanup after disconnection
void DefaultStorageStage::cleanup() {
  LOG_TRACE("Enter");

  if (handler_) {
    handler_->destroy();
    handler_ = nullptr;
  }
  LOG_TRACE("Exit");
}

void DefaultStorageStage::handle_event(StageEvent *event) {
  LOG_TRACE("Enter\n");
  TimerStat timerStat(*query_metric_);

  StorageEvent *storage_event = static_cast<StorageEvent *>(event);
  CompletionCallback *cb = new (std::nothrow) CompletionCallback(this, nullptr);
  if (cb == nullptr) {
    LOG_ERROR("Failed to new callback for SessionEvent");
    storage_event->done_immediate();
    return;
  }
  storage_event->push_callback(cb);

  Query *sql = storage_event->exe_event()->sqls();

  SessionEvent *session_event = storage_event->exe_event()->sql_event()->session_event();

  Session *session = session_event->get_client()->session;
  const char *current_db = session->get_current_db().c_str();

  Trx *current_trx = session->current_trx();

  RC rc = RC::SUCCESS;

  char response[256];
  switch (sql->flag)
  {
  case SCF_INSERT: { // insert into
      const Inserts &inserts = sql->sstr.insertion;
      const char *table_name = inserts.relation_name;
      rc = handler_->insert_record(current_trx, current_db, table_name, inserts.value_num, inserts.values);
      snprintf(response, sizeof(response), "%s\n", rc == RC::SUCCESS ? "SUCCESS" : "FAILURE");
    }
    break;
  case SCF_UPDATE: {
      const Updates &updates = sql->sstr.update;
      const char *table_name = updates.relation_name;
      const char *field_name = updates.attribute_name;
      int updated_count = 0;
      rc = handler_->update_record(current_trx, current_db, table_name, field_name, &updates.value,
                                   updates.condition_num, updates.conditions, &updated_count);
      snprintf(response, sizeof(response), "%s\n", rc == RC::SUCCESS ? "SUCCESS" : "FAILURE");
    }
    break;
  case SCF_DELETE: {
      const Deletes &deletes = sql->sstr.deletion;
      const char *table_name = deletes.relation_name;
      int deleted_count = 0;
      rc = handler_->delete_record(current_trx, current_db, table_name, deletes.condition_num, deletes.conditions, &deleted_count);
      snprintf(response, sizeof(response), "%s\n", rc == RC::SUCCESS ? "SUCCESS" : "FAILURE");
    }
    break;
  case SCF_CREATE_TABLE: { // create table
      const CreateTable &create_table = sql->sstr.create_table;
      rc = handler_->create_table(current_db, create_table.relation_name, 
              create_table.attribute_count, create_table.attributes);
      snprintf(response, sizeof(response), "%s\n", rc == RC::SUCCESS ? "SUCCESS" : "FAILURE");
    }
    break;
  case SCF_CREATE_INDEX: {
      const CreateIndex &create_index = sql->sstr.create_index;
      rc = handler_->create_index(current_trx, current_db, create_index.relation_name, 
                                  create_index.index_name, create_index.attribute_name);
      snprintf(response, sizeof(response), "%s\n", rc == RC::SUCCESS ? "SUCCESS" : "FAILURE");
    }
    break;

  case SCF_SHOW_TABLES: {
      Db *db = handler_->find_db(current_db);
      if (nullptr == db) {
        snprintf(response, sizeof(response), "No such database: %s\n", current_db);
      } else {
        std::vector<std::string> all_tables;
        db->all_tables(all_tables);
        if (all_tables.empty()) {
          snprintf(response, sizeof(response), "No table\n");
        } else {
          std::stringstream ss;
          for (const auto &table: all_tables) {
            ss << table << std::endl;
          }
          snprintf(response, sizeof(response), "%s\n", ss.str().c_str());
        }
      }
    }
    break;
  case SCF_DESC_TABLE: {
      const char *table_name = sql->sstr.desc_table.relation_name;
      Table *table = handler_->find_table(current_db, table_name);
      std::stringstream ss;
      if (table != nullptr) {
        table->table_meta().desc(ss);
      } else {
        ss << "No such table: " << table_name << std::endl;
      }
      snprintf(response, sizeof(response), "%s", ss.str().c_str());
    }
    break;

  case SCF_LOAD_DATA: {
      /*
        从文件导入数据，如果做性能测试，需要保持这些代码可以正常工作
        load data infile `your/file/path` into table `table-name`;
       */
      const char *table_name = sql->sstr.load_data.relation_name;
      const char *file_name = sql->sstr.load_data.file_name;
      std::string result = load_data(current_db, table_name, file_name);
      snprintf(response, sizeof(response), "%s", result.c_str());
    }
    break;
  default:
      snprintf(response, sizeof(response), "Unsupported sql: %d\n", sql->flag);
      break;
  }

  if (rc == RC::SUCCESS && !session->is_trx_multi_operation_mode()) {
    rc = current_trx->commit();
    if (rc != RC::SUCCESS) {
      LOG_ERROR("Failed to commit trx. rc=%d:%s", rc, strrc(rc));
    }
  }

  session_event->set_response(response);
  event->done_immediate();

  LOG_TRACE("Exit\n");
}

void DefaultStorageStage::callback_event(StageEvent *event,
                                        CallbackContext *context) {
  LOG_TRACE("Enter\n");
  StorageEvent *storage_event = static_cast<StorageEvent *>(event);
  storage_event->exe_event()->done_immediate();
  LOG_TRACE("Exit\n");
  return;
}

/**
 * 从文件中导入数据时使用。尝试向表中插入解析后的一行数据。
 * @param table  要导入的表
 * @param file_values 从文件中读取到的一行数据，使用分隔符拆分后的几个字段值
 * @param record_values Table::insert_record使用的参数，为了防止频繁的申请内存
 * @param errmsg 如果出现错误，通过这个参数返回错误信息
 * @return 成功返回RC::SUCCESS
 */
RC insert_record_from_file(Table *table, std::vector<std::string> &file_values, 
                std::vector<Value> &record_values, std::stringstream &errmsg) {

  const int field_num = record_values.size();
  const int sys_field_num = table->table_meta().sys_field_num();

  if (file_values.size() < record_values.size()) {
    return RC::SCHEMA_FIELD_MISSING;
  }

  RC rc = RC::SUCCESS;

  std::stringstream deserialize_stream;
  for (int i = 0; i < field_num && RC::SUCCESS == rc; i++) {
    const FieldMeta *field = table->table_meta().field(i + sys_field_num);
    
    std::string &file_value = file_values[i];
    common::strip(file_value);

    switch (field->type()) {
      case INTS: {
        deserialize_stream.clear(); // 清理stream的状态，防止多次解析出现异常
        deserialize_stream.str(file_value);

        int int_value;
        deserialize_stream >> int_value;
        if (!deserialize_stream || !deserialize_stream.eof()) {
          errmsg << "need an integer but got '" << file_values[i] 
                 << "' (field index:" << i << ")";

          rc = RC::SCHEMA_FIELD_TYPE_MISMATCH;
        } else {
          value_init_integer(&record_values[i], int_value);
        }
      }

      break;
      case FLOATS: {
        deserialize_stream.clear();
        deserialize_stream.str(file_value);

        float float_value;
        deserialize_stream >> float_value;
        if (!deserialize_stream || !deserialize_stream.eof()) {
          errmsg << "need a float number but got '" << file_values[i] 
              << "'(field index:" << i << ")"; 
          rc = RC::SCHEMA_FIELD_TYPE_MISMATCH;
        } else {
          value_init_float(&record_values[i], float_value);
        }
      }
      break;
      case CHARS: {
        value_init_string(&record_values[i], file_value.c_str());
      }
      break;
      default: {
        errmsg << "Unsupported field type to loading: " << field->type();
        rc = RC::SCHEMA_FIELD_TYPE_MISMATCH;
      }
      break;
    }
  }

  if (RC::SUCCESS == rc) {
    rc = table->insert_record(nullptr, field_num, record_values.data());
    if (rc != RC::SUCCESS) {
      errmsg << "insert failed.";
    }
  }
  for (int i = 0; i < field_num; i++) {
    value_destroy(&record_values[i]);
  }
  return rc;
}

std::string DefaultStorageStage::load_data(const char *db_name, 
          const char *table_name, const char *file_name) {

  std::stringstream result_string;
  Table *table = handler_->find_table(db_name, table_name);
  if (nullptr == table) {
    result_string << "No such table " << db_name << "." << table_name << std::endl;
    return result_string.str();
  }

  std::fstream fs;
  fs.open(file_name, std::ios_base::in | std::ios_base::binary);
  if (!fs.is_open()) {
    result_string << "Failed to open file: " << file_name 
                  << ". system error=" << strerror(errno) << std::endl;
    return result_string.str();
  }

  struct timespec begin_time;
  clock_gettime(CLOCK_MONOTONIC, &begin_time);
  const int sys_field_num = table->table_meta().sys_field_num();
  const int field_num = table->table_meta().field_num() - sys_field_num;

  std::vector<Value> record_values(field_num);
  std::string line;
  std::vector<std::string> file_values;
  const std::string delim("|");
  int line_num = 0;
  int insertion_count = 0;
  RC rc = RC::SUCCESS;
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
      result_string << "Line:" << line_num << " insert record failed:" 
          << errmsg.str() << ". error:" << strrc(rc) << std::endl;
    } else {
      insertion_count++;
    }
  }
  fs.close();

  struct timespec end_time;
  clock_gettime(CLOCK_MONOTONIC, &end_time);
  long cost_nano = (end_time.tv_sec - begin_time.tv_sec) * 1000000000L 
                    + (end_time.tv_nsec - begin_time.tv_nsec);
  if (RC::SUCCESS == rc) {
    result_string << strrc(rc) << ". total " << line_num << " line(s) handled and " 
                  << insertion_count << " record(s) loaded, total cost " << cost_nano / 1000000000.0 
                  << " second(s)" << std::endl;
  }
  return result_string.str();
}