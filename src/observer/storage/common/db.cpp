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
// Created by Meiyi & Longda & Wangyunlai on 2021/5/12.
//

#include "storage/common/db.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <vector>

#include "common/log/log.h"
#include "common/os/path.h"
#include "common/lang/string.h"
#include "storage/common/table_meta.h"
#include "storage/common/table.h"
#include "storage/common/meta_util.h"
#include "storage/trx/trx.h"
#include "storage/clog/clog.h"

Db::~Db()
{
  for (auto &iter : opened_tables_) {
    delete iter.second;
  }
  LOG_INFO("Db has been closed: %s", name_.c_str());
}

RC Db::init(const char *name, const char *dbpath)
{

  if (common::is_blank(name)) {
    LOG_ERROR("Failed to init DB, name cannot be empty");
    return RC::INVALID_ARGUMENT;
  }

  if (!common::is_directory(dbpath)) {
    LOG_ERROR("Failed to init DB, path is not a directory: %s", dbpath);
    return RC::GENERIC_ERROR;
  }

  clog_manager_ = new CLogManager(dbpath);
  if (clog_manager_ == nullptr) {
    LOG_ERROR("Failed to init CLogManager.");
    return RC::GENERIC_ERROR;
  }

  name_ = name;
  path_ = dbpath;

  return open_all_tables();
}

RC Db::create_table(const char *table_name, int attribute_count, const AttrInfo *attributes)
{
  RC rc = RC::SUCCESS;
  // check table_name
  if (opened_tables_.count(table_name) != 0) {
    LOG_WARN("%s has been opened before.", table_name);
    return RC::SCHEMA_TABLE_EXIST;
  }

  // 文件路径可以移到Table模块
  std::string table_file_path = table_meta_file(path_.c_str(), table_name);
  Table *table = new Table();
  rc = table->create(table_file_path.c_str(), table_name, path_.c_str(), attribute_count, attributes, get_clog_manager());
  if (rc != RC::SUCCESS) {
    LOG_ERROR("Failed to create table %s.", table_name);
    delete table;
    return rc;
  }

  opened_tables_[table_name] = table;
  LOG_INFO("Create table success. table name=%s", table_name);
  return RC::SUCCESS;
}

Table *Db::find_table(const char *table_name) const
{
  std::unordered_map<std::string, Table *>::const_iterator iter = opened_tables_.find(table_name);
  if (iter != opened_tables_.end()) {
    return iter->second;
  }
  return nullptr;
}

RC Db::open_all_tables()
{
  std::vector<std::string> table_meta_files;
  int ret = common::list_file(path_.c_str(), TABLE_META_FILE_PATTERN, table_meta_files);
  if (ret < 0) {
    LOG_ERROR("Failed to list table meta files under %s.", path_.c_str());
    return RC::IOERR;
  }

  RC rc = RC::SUCCESS;
  for (const std::string &filename : table_meta_files) {
    Table *table = new Table();
    rc = table->open(filename.c_str(), path_.c_str(), clog_manager_);
    if (rc != RC::SUCCESS) {
      delete table;
      LOG_ERROR("Failed to open table. filename=%s", filename.c_str());
      return rc;
    }

    if (opened_tables_.count(table->name()) != 0) {
      delete table;
      LOG_ERROR("Duplicate table with difference file name. table=%s, the other filename=%s",
          table->name(),
          filename.c_str());
      return RC::GENERIC_ERROR;
    }

    opened_tables_[table->name()] = table;
    LOG_INFO("Open table: %s, file: %s", table->name(), filename.c_str());
  }

  LOG_INFO("All table have been opened. num=%d", opened_tables_.size());
  return rc;
}

const char *Db::name() const
{
  return name_.c_str();
}

void Db::all_tables(std::vector<std::string> &table_names) const
{
  for (const auto &table_item : opened_tables_) {
    table_names.emplace_back(table_item.first);
  }
}

RC Db::sync()
{
  RC rc = RC::SUCCESS;
  for (const auto &table_pair : opened_tables_) {
    Table *table = table_pair.second;
    rc = table->sync();
    if (rc != RC::SUCCESS) {
      LOG_ERROR("Failed to flush table. table=%s.%s, rc=%d:%s", name_.c_str(), table->name(), rc, strrc(rc));
      return rc;
    }
    LOG_INFO("Successfully sync table db:%s, table:%s.", name_.c_str(), table->name());
  }
  LOG_INFO("Successfully sync db. db=%s", name_.c_str());
  return rc;
}

RC Db::recover()
{
  RC rc = RC::SUCCESS;
  if ((rc = clog_manager_->recover()) == RC::SUCCESS) {
    uint32_t max_trx_id = 0;
    CLogMTRManager *mtr_manager = clog_manager_->get_mtr_manager();
    for (auto it = mtr_manager->log_redo_list.begin(); it != mtr_manager->log_redo_list.end(); it++) {
      CLogRecord *clog_record = *it;
      if (clog_record->get_log_type() != CLogType::REDO_INSERT && clog_record->get_log_type() != CLogType::REDO_DELETE) {
        delete clog_record;
        continue;
      }
      auto find_iter = mtr_manager->trx_commited.find(clog_record->get_trx_id());
      if (find_iter == mtr_manager->trx_commited.end()) {
        LOG_ERROR("CLog record without commit message! "); // unexpected error
        delete clog_record;
        return RC::GENERIC_ERROR;
      } else if (find_iter->second == false ) {
        delete clog_record;
        continue;
      }

      Table *table = find_table(clog_record->log_record_.ins.table_name_);
      if (table == nullptr) {
        delete clog_record;
        continue;
      }

      switch(clog_record->get_log_type()) {
        case CLogType::REDO_INSERT: {
          char *record_data = new char[clog_record->log_record_.ins.data_len_];
          memcpy(record_data, clog_record->log_record_.ins.data_, clog_record->log_record_.ins.data_len_);
          Record record;
          record.set_data(record_data);
          record.set_rid(clog_record->log_record_.ins.rid_);

          rc = table->recover_insert_record(&record);
          delete[] record_data;
        } break;
        case CLogType::REDO_DELETE: {
          Record record;
          record.set_rid(clog_record->log_record_.del.rid_);
          rc = table->recover_delete_record(&record);
        } break;
        default: {
          rc = RC::SUCCESS;
        }
      }

      if (rc != RC::SUCCESS) {
        LOG_ERROR("Failed to recover. rc=%d:%s", rc, strrc(rc));
        break;
      }
      
      if (max_trx_id < clog_record->get_trx_id()) {
        max_trx_id = clog_record->get_trx_id();
      }
      delete clog_record;
    }

    if (rc == RC::SUCCESS && max_trx_id > 0) {
      Trx::set_trx_id(max_trx_id);
    }
  }

  return rc;
}

CLogManager *Db::get_clog_manager() {
  return clog_manager_;
}