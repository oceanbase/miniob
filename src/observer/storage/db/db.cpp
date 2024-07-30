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

#include "storage/db/db.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <vector>
#include <filesystem>

#include "common/lang/string.h"
#include "common/log/log.h"
#include "common/os/path.h"
#include "common/global_context.h"
#include "storage/common/meta_util.h"
#include "storage/table/table.h"
#include "storage/table/table_meta.h"
#include "storage/trx/trx.h"
#include "storage/clog/disk_log_handler.h"
#include "storage/clog/integrated_log_replayer.h"

using namespace common;

Db::~Db()
{
  for (auto &iter : opened_tables_) {
    delete iter.second;
  }

  if (log_handler_) {
    // 停止日志并等待写入完成
    log_handler_->stop();
    log_handler_->await_termination();
    log_handler_.reset();
  }
  LOG_INFO("Db has been closed: %s", name_.c_str());
}

RC Db::init(const char *name, const char *dbpath, const char *trx_kit_name, const char *log_handler_name)
{
  RC rc = RC::SUCCESS;

  if (common::is_blank(name)) {
    LOG_ERROR("Failed to init DB, name cannot be empty");
    return RC::INVALID_ARGUMENT;
  }

  if (!filesystem::is_directory(dbpath)) {
    LOG_ERROR("Failed to init DB, path is not a directory: %s", dbpath);
    return RC::INVALID_ARGUMENT;
  }

  TrxKit *trx_kit = TrxKit::create(trx_kit_name);
  if (trx_kit == nullptr) {
    LOG_ERROR("Failed to create trx kit: %s", trx_kit_name);
    return RC::INVALID_ARGUMENT;
  }

  trx_kit_.reset(trx_kit);

  buffer_pool_manager_ = make_unique<BufferPoolManager>();
  auto dblwr_buffer    = make_unique<DiskDoubleWriteBuffer>(*buffer_pool_manager_);

  const char      *double_write_buffer_filename  = "dblwr.db";
  filesystem::path double_write_buffer_file_path = filesystem::path(dbpath) / double_write_buffer_filename;
  rc                                             = dblwr_buffer->open_file(double_write_buffer_file_path.c_str());
  if (OB_FAIL(rc)) {
    LOG_ERROR("Failed to open double write buffer file. file=%s, rc=%s",
              double_write_buffer_file_path.c_str(), strrc(rc));
    return rc;
  }

  rc = buffer_pool_manager_->init(std::move(dblwr_buffer));
  if (OB_FAIL(rc)) {
    LOG_ERROR("Failed to init buffer pool manager. dbpath=%s, rc=%s", dbpath, strrc(rc));
    return rc;
  }

  filesystem::path clog_path       = filesystem::path(dbpath) / "clog";
  LogHandler      *tmp_log_handler = nullptr;
  rc                               = LogHandler::create(log_handler_name, tmp_log_handler);
  if (OB_FAIL(rc)) {
    LOG_ERROR("Failed to create log handler: %s", log_handler_name);
    return rc;
  }
  log_handler_.reset(tmp_log_handler);

  rc = log_handler_->init(clog_path.c_str());
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to init log handler. dbpath=%s, rc=%s", dbpath, strrc(rc));
    return rc;
  }

  name_ = name;
  path_ = dbpath;

  // 加载数据库本身的元数据
  rc = init_meta();
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to init meta. dbpath=%s, rc=%s", dbpath, strrc(rc));
    return rc;
  }

  // 打开所有表
  // 在实际生产数据库中，直接打开所有表，可能耗时会比较长
  rc = open_all_tables();
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to open all tables. dbpath=%s, rc=%s", dbpath, strrc(rc));
    return rc;
  }

  rc = init_dblwr_buffer();
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to init dblwr buffer. rc = %s", strrc(rc));
    return rc;
  }

  // 尝试恢复数据库，重做redo日志
  rc = recover();
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to recover db. dbpath=%s, rc=%s", dbpath, strrc(rc));
    return rc;
  }

  return rc;
}

RC Db::create_table(const char *table_name, span<const AttrInfoSqlNode> attributes, const StorageFormat storage_format)
{
  RC rc = RC::SUCCESS;
  // check table_name
  if (opened_tables_.count(table_name) != 0) {
    LOG_WARN("%s has been opened before.", table_name);
    return RC::SCHEMA_TABLE_EXIST;
  }

  // 文件路径可以移到Table模块
  string  table_file_path = table_meta_file(path_.c_str(), table_name);
  Table  *table           = new Table();
  int32_t table_id        = next_table_id_++;
  rc = table->create(this, table_id, table_file_path.c_str(), table_name, path_.c_str(), attributes, storage_format);
  if (rc != RC::SUCCESS) {
    LOG_ERROR("Failed to create table %s.", table_name);
    delete table;
    return rc;
  }

  opened_tables_[table_name] = table;
  LOG_INFO("Create table success. table name=%s, table_id:%d", table_name, table_id);
  return RC::SUCCESS;
}

Table *Db::find_table(const char *table_name) const
{
  unordered_map<string, Table *>::const_iterator iter = opened_tables_.find(table_name);
  if (iter != opened_tables_.end()) {
    return iter->second;
  }
  return nullptr;
}

Table *Db::find_table(int32_t table_id) const
{
  for (auto pair : opened_tables_) {
    if (pair.second->table_id() == table_id) {
      return pair.second;
    }
  }
  return nullptr;
}

RC Db::open_all_tables()
{
  vector<string> table_meta_files;

  int ret = list_file(path_.c_str(), TABLE_META_FILE_PATTERN, table_meta_files);
  if (ret < 0) {
    LOG_ERROR("Failed to list table meta files under %s.", path_.c_str());
    return RC::IOERR_READ;
  }

  RC rc = RC::SUCCESS;
  for (const string &filename : table_meta_files) {
    Table *table = new Table();
    rc           = table->open(this, filename.c_str(), path_.c_str());
    if (rc != RC::SUCCESS) {
      delete table;
      LOG_ERROR("Failed to open table. filename=%s", filename.c_str());
      return rc;
    }

    if (opened_tables_.count(table->name()) != 0) {
      LOG_ERROR("Duplicate table with difference file name. table=%s, the other filename=%s",
          table->name(), filename.c_str());
      // 在这里原本先删除table后调用table->name()方法，犯了use-after-free的错误
      delete table;
      return RC::INTERNAL;
    }

    if (table->table_id() >= next_table_id_) {
      next_table_id_ = table->table_id() + 1;
    }
    opened_tables_[table->name()] = table;
    LOG_INFO("Open table: %s, file: %s", table->name(), filename.c_str());
  }

  LOG_INFO("All table have been opened. num=%d", opened_tables_.size());
  return rc;
}

const char *Db::name() const { return name_.c_str(); }

void Db::all_tables(vector<string> &table_names) const
{
  for (const auto &table_item : opened_tables_) {
    table_names.emplace_back(table_item.first);
  }
}

RC Db::sync()
{
  RC rc = RC::SUCCESS;
  // 调用所有表的sync函数刷新数据到磁盘
  for (const auto &table_pair : opened_tables_) {
    Table *table = table_pair.second;
    rc           = table->sync();
    if (rc != RC::SUCCESS) {
      LOG_ERROR("Failed to flush table. table=%s.%s, rc=%d:%s", name_.c_str(), table->name(), rc, strrc(rc));
      return rc;
    }
    LOG_INFO("Successfully sync table db:%s, table:%s.", name_.c_str(), table->name());
  }

  auto dblwr_buffer = static_cast<DiskDoubleWriteBuffer *>(buffer_pool_manager_->get_dblwr_buffer());
  rc                = dblwr_buffer->flush_page();
  LOG_INFO("double write buffer flush pages ret=%s", strrc(rc));

  /*
  在sync期间，不允许有未完成的事务，也不允许开启新的事物。
  这个约束不是从程序层面处理的，而是认为的约束。
  */
  LSN current_lsn = log_handler_->current_lsn();
  rc              = log_handler_->wait_lsn(current_lsn);
  if (OB_FAIL(rc)) {
    LOG_ERROR("Failed to wait lsn. lsn=%ld, rc=%d:%s", current_lsn, rc, strrc(rc));
    return rc;
  }

  check_point_lsn_ = current_lsn;
  rc               = flush_meta();
  if (OB_FAIL(rc)) {
    LOG_ERROR("Failed to flush meta. db=%s, rc=%d:%s", name_.c_str(), rc, strrc(rc));
    return rc;
  }
  LOG_INFO("Successfully sync db. db=%s", name_.c_str());
  return rc;
}

RC Db::recover()
{
  LOG_TRACE("db recover begin. check_point_lsn=%d", check_point_lsn_);

  LogReplayer *trx_log_replayer = trx_kit_->create_log_replayer(*this, *log_handler_);
  if (trx_log_replayer == nullptr) {
    LOG_ERROR("Failed to create trx log replayer.");
    return RC::INTERNAL;
  }

  IntegratedLogReplayer log_replayer(*buffer_pool_manager_, unique_ptr<LogReplayer>(trx_log_replayer));
  RC                    rc = log_handler_->replay(log_replayer, check_point_lsn_ /*start_lsn*/);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to replay log. rc=%s", strrc(rc));
    return rc;
  }

  rc = log_handler_->start();
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to start log handler. rc=%s", strrc(rc));
    return rc;
  }

  rc = log_replayer.on_done();
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to on_done. rc=%s", strrc(rc));
    return rc;
  }

  LOG_INFO("Successfully recover db. db=%s checkpoint_lsn=%d", name_.c_str(), check_point_lsn_);
  return rc;
}

RC Db::init_meta()
{
  filesystem::path db_meta_file_path = db_meta_file(path_.c_str(), name_.c_str());
  if (!filesystem::exists(db_meta_file_path)) {
    check_point_lsn_ = 0;
    LOG_INFO("Db meta file not exist. db=%s, file=%s", name_.c_str(), db_meta_file_path.c_str());
    return RC::SUCCESS;
  }

  RC  rc = RC::SUCCESS;
  int fd = open(db_meta_file_path.c_str(), O_RDONLY);
  if (fd < 0) {
    LOG_ERROR("Failed to open db meta file. db=%s, file=%s, errno=%s", 
              name_.c_str(), db_meta_file_path.c_str(), strerror(errno));
    return RC::IOERR_READ;
  }

  char buffer[1024];
  int  n = read(fd, buffer, sizeof(buffer));
  if (n < 0) {
    LOG_ERROR("Failed to read db meta file. db=%s, file=%s, errno=%s", 
              name_.c_str(), db_meta_file_path.c_str(), strerror(errno));
    rc = RC::IOERR_READ;
  } else {
    if (n >= static_cast<int>(sizeof(buffer))) {
      LOG_WARN("Db meta file is too large. db=%s, file=%s, buffer size=%ld", 
               name_.c_str(), db_meta_file_path.c_str(), sizeof(buffer));
      return RC::IOERR_TOO_LONG;
    }

    buffer[n]        = '\0';
    check_point_lsn_ = atoll(buffer);  // 当前元数据就这一个数字
    LOG_INFO("Successfully read db meta file. db=%s, file=%s, check_point_lsn=%ld", 
             name_.c_str(), db_meta_file_path.c_str(), check_point_lsn_);
  }
  close(fd);

  return rc;
}

RC Db::flush_meta()
{
  // 将数据库元数据刷新到磁盘
  // 先创建一个临时文件，将元数据写入临时文件
  // 然后再将临时文件修改为正式文件

  filesystem::path meta_file_path      = db_meta_file(path_.c_str(), name_.c_str());  // 正式文件名
  filesystem::path temp_meta_file_path = meta_file_path;                              // 临时文件名
  temp_meta_file_path += ".tmp";

  RC  rc = RC::SUCCESS;
  int fd = open(temp_meta_file_path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
  if (fd < 0) {
    LOG_ERROR("Failed to open db meta file. db=%s, file=%s, errno=%s", 
              name_.c_str(), temp_meta_file_path.c_str(), strerror(errno));
    return RC::IOERR_WRITE;
  }

  string buffer = std::to_string(check_point_lsn_);
  int    n      = write(fd, buffer.c_str(), buffer.size());
  if (n < 0) {
    LOG_ERROR("Failed to write db meta file. db=%s, file=%s, errno=%s", 
              name_.c_str(), temp_meta_file_path.c_str(), strerror(errno));
    rc = RC::IOERR_WRITE;
  } else if (n != static_cast<int>(buffer.size())) {
    LOG_ERROR("Failed to write db meta file. db=%s, file=%s, buffer size=%ld, write size=%d", 
              name_.c_str(), temp_meta_file_path.c_str(), buffer.size(), n);
    rc = RC::IOERR_WRITE;
  } else {
    error_code ec;
    filesystem::rename(temp_meta_file_path, meta_file_path, ec);
    if (ec) {
      LOG_ERROR("Failed to rename db meta file. db=%s, file=%s, errno=%s", 
                name_.c_str(), temp_meta_file_path.c_str(), ec.message().c_str());
      rc = RC::IOERR_WRITE;
    } else {

      LOG_INFO("Successfully write db meta file. db=%s, file=%s, check_point_lsn=%ld", 
               name_.c_str(), temp_meta_file_path.c_str(), check_point_lsn_);
    }
  }

  return rc;
}

RC Db::init_dblwr_buffer()
{
  auto dblwr_buffer = static_cast<DiskDoubleWriteBuffer *>(buffer_pool_manager_->get_dblwr_buffer());
  RC   rc           = dblwr_buffer->recover();
  if (OB_FAIL(rc)) {
    LOG_ERROR("fail to recover in dblwr buffer");
    return rc;
  }

  return RC::SUCCESS;
}

LogHandler        &Db::log_handler() { return *log_handler_; }
BufferPoolManager &Db::buffer_pool_manager() { return *buffer_pool_manager_; }
TrxKit            &Db::trx_kit() { return *trx_kit_; }
