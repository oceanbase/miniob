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


#include "storage/default/default_handler.h"

#include <string>

#include "common/os/path.h"
#include "common/log/log.h"
#include "common/lang/string.h"
#include "storage/common/record_manager.h"
#include "storage/common/bplus_tree.h"
#include "storage/common/table.h"
#include "storage/common/condition_filter.h"

DefaultHandler &DefaultHandler::get_default() {
  static DefaultHandler handler;
  return handler;
}

DefaultHandler::DefaultHandler() {
}

DefaultHandler::~DefaultHandler() noexcept {
  destroy();
}

RC DefaultHandler::init(const char *base_dir) {
  // 检查目录是否存在，或者创建
  std::string tmp(base_dir);
  tmp += "/db";
  if (!common::check_directory(tmp)) {
    LOG_ERROR("Cannot access base dir: %s. msg=%d:%s", tmp.c_str(), errno, strerror(errno));
    return RC::GENERIC_ERROR;
  }

  base_dir_ = base_dir;
  db_dir_ = tmp + "/";

  LOG_INFO("Default handler init with %s success", base_dir);
  return RC::SUCCESS;
}

void DefaultHandler::destroy() {
  sync();

  for (const auto & iter : opened_dbs_) {
    delete iter.second;
  }
  opened_dbs_.clear();
}

RC DefaultHandler::create_db(const char *dbname) {
  if (nullptr == dbname || common::is_blank(dbname)) {
    LOG_WARN("Invalid db name");
    return RC::INVALID_ARGUMENT;
  }

  // 如果对应名录已经存在，返回错误
  std::string dbpath = db_dir_ + dbname;
  if (common::is_directory(dbpath.c_str())) {
    LOG_WARN("Db already exists: %s", dbname);
    return RC::SCHEMA_DB_EXIST;
  }

  if (!common::check_directory(dbpath)) {
    LOG_ERROR("Create db fail: %s", dbpath.c_str());
    return RC::GENERIC_ERROR; // io error
  }
  return RC::SUCCESS;
}

RC DefaultHandler::drop_db(const char *dbname) {
  return RC::GENERIC_ERROR;
}

RC DefaultHandler::open_db(const char *dbname) {
  if (nullptr == dbname || common::is_blank(dbname)) {
    LOG_WARN("Invalid db name");
    return RC::INVALID_ARGUMENT;
  }

  if (opened_dbs_.find(dbname) != opened_dbs_.end()) {
    return RC::SUCCESS;
  }

  std::string dbpath = db_dir_ + dbname;
  if (!common::is_directory(dbpath.c_str())) {
    return RC::SCHEMA_DB_NOT_EXIST;
  }

  // open db
  Db *db = new Db();
  RC ret = RC::SUCCESS;
  if ((ret = db->init(dbname, dbpath.c_str())) != RC::SUCCESS) {
    LOG_ERROR("Failed to open db: %s. error=%d", dbname, ret);
  }
  opened_dbs_[dbname] = db;
  return RC::SUCCESS;
}

RC DefaultHandler::close_db(const char *dbname) {
  return RC::GENERIC_ERROR;
}

RC DefaultHandler::execute(const char *sql) {
  return RC::GENERIC_ERROR;
}

RC DefaultHandler::create_table(const char *dbname, const char *relation_name, int attribute_count, const AttrInfo *attributes) {
  Db *db = find_db(dbname);
  if (db == nullptr) {
    return RC::SCHEMA_DB_NOT_OPENED;
  }
  return db->create_table(relation_name, attribute_count, attributes);
}

RC DefaultHandler::drop_table(const char *dbname, const char *relation_name) {
  return RC::GENERIC_ERROR;
}

RC DefaultHandler::create_index(Trx *trx, const char *dbname, const char *relation_name, const char *index_name, const char *attribute_name) {
  Table *table = find_table(dbname, relation_name);
  if (nullptr == table) {
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }
  return table->create_index(trx, index_name, attribute_name);
}

RC DefaultHandler::drop_index(Trx *trx, const char *dbname, const char *relation_name, const char *index_name) {

  return RC::GENERIC_ERROR;
}

RC DefaultHandler::insert_record(Trx *trx, const char *dbname, const char *relation_name, int value_num, const Value *values) {
  Table *table = find_table(dbname, relation_name);
  if (nullptr == table) {
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }

  return table->insert_record(trx, value_num, values);
}
RC DefaultHandler::delete_record(Trx *trx, const char *dbname, const char *relation_name,
                                 int condition_num, const Condition *conditions, int *deleted_count) {
  Table *table = find_table(dbname, relation_name);
  if (nullptr == table) {
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }

  CompositeConditionFilter condition_filter;
  RC rc = condition_filter.init(*table, conditions, condition_num);
  if (rc != RC::SUCCESS) {
    return rc;
  }
  return table->delete_record(trx, &condition_filter, deleted_count);
}

RC DefaultHandler::update_record(Trx *trx, const char *dbname, const char *relation_name, const char *attribute_name, const Value *value,
                          int condition_num, const Condition *conditions, int *updated_count) {
  Table *table = find_table(dbname, relation_name);
  if (nullptr == table) {
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }

  return table->update_record(trx, attribute_name, value, condition_num, conditions, updated_count);
}

Db *DefaultHandler::find_db(const char *dbname) const {
  std::map<std::string, Db*>::const_iterator iter = opened_dbs_.find(dbname);
  if (iter == opened_dbs_.end()) {
    return nullptr;
  }
  return iter->second;
}

Table *DefaultHandler::find_table(const char *dbname, const char *table_name) const {
  if (dbname == nullptr || table_name == nullptr) {
    LOG_WARN("Invalid argument. dbname=%p, table_name=%p", dbname, table_name);
    return nullptr;
  }
  Db *db = find_db(dbname);
  if (nullptr == db) {
    return nullptr;
  }

  return db->find_table(table_name);
}

RC DefaultHandler::sync() {
  RC rc = RC::SUCCESS;
  for (const auto & db_pair: opened_dbs_) {
    Db *db = db_pair.second;
    rc = db->sync();
    if (rc != RC::SUCCESS) {
      LOG_ERROR("Failed to sync db. name=%s, rc=%d:%s", db->name(), rc, strrc(rc));
      return rc;
    }
  }
  return rc;
}