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
// Created by wangyunlai.wyl on 2024/03/01
//

#include <filesystem>
#include <memory>
#include <vector>
#include <string>

#include "gtest/gtest.h"
#include "storage/db/db.h"
#include "storage/table/table.h"
#include "storage/record/record.h"
#include "storage/trx/mvcc_trx.h"

using namespace std;
using namespace common;

TEST(MvccTrxLog, wal)
{
  /*
  创建一个数据库，建一些表，插入一些数据。
  然后等所有日志都落地，将文件都复制到另一个目录。此时buffer pool 应该都没落地。
  使用新的目录初始化一个新的数据库，然后检查数据是否一致。
  */
  filesystem::path test_directory("mvcc_trx_log_test");
  filesystem::remove_all(test_directory);
  filesystem::create_directory(test_directory);

  const char *dbname = "test_db";
  const char *dbname2 = "test_db2";
  filesystem::path db_path = test_directory / dbname;
  filesystem::path db_path2 = test_directory / dbname2;
  const char *trx_kit_name = "mvcc";

  auto db = make_unique<Db>();
  ASSERT_EQ(RC::SUCCESS, db->init(dbname, db_path.c_str(), trx_kit_name));

  const int table_num = 10;
  vector<string> table_names;
  for (int i = 0; i < table_num; i++) {
    table_names.push_back("table_" + to_string(i));
  }

  const int field_num = 10;
  vector<AttrInfoSqlNode> attr_infos;
  for (int i = 0; i < field_num; i++) {
    AttrInfoSqlNode attr_info;
    attr_info.name = string("field_") + to_string(i);
    attr_info.type = AttrType::INTS;
    attr_info.length = 4;
    attr_infos.push_back(attr_info);
  }

  for (const string &table_name : table_names) {
    ASSERT_EQ(RC::SUCCESS, db->create_table(table_name.c_str(), attr_infos.size(), attr_infos.data()));
    ASSERT_EQ(RC::SUCCESS, db->sync());
  }

  TrxKit &trx_kit = db->trx_kit();
  Trx *trx = trx_kit.create_trx(db->log_handler());
  const int insert_num = 10000;
  for (int i = 0; i < insert_num; i++) {
    ASSERT_NE(trx, nullptr);

    for (const string &table_name : table_names) {
      Table *table = db->find_table(table_name.c_str());
      ASSERT_NE(table, nullptr);
      
      Record record;

      vector<Value> values(field_num);
      for (Value &value : values) {
        value.set_int(i);
      }

      ASSERT_EQ(RC::SUCCESS, table->make_record(values.size(), values.data(), record));

      ASSERT_EQ(RC::SUCCESS, trx->insert_record(table, record));
    }

    ASSERT_EQ(RC::SUCCESS, trx->commit());
  }

  DiskLogHandler &log_handler = static_cast<DiskLogHandler &>(db->log_handler());
  LSN current_lsn = log_handler.current_lsn();
  ASSERT_NE(RC::SUCCESS, log_handler.wait_lsn(current_lsn));

  // copy all files from db to db2
  filesystem::copy(db_path, db_path2, filesystem::copy_options::recursive);

  auto db2 = make_unique<Db>();
  ASSERT_EQ(RC::SUCCESS, db2->init(dbname2, db_path2.c_str(), trx_kit_name));
  vector<string> table_names2;
  db2->all_tables(table_names2);
  ASSERT_EQ(table_names, table_names2);

  // count each table's record number and take a check
  for (const string &table_name : table_names2) {

    Table *table2 = db2->find_table(table_name.c_str());
    ASSERT_NE(table2, nullptr);

    RecordFileScanner scanner2;
    ASSERT_EQ(RC::SUCCESS, table2->get_record_scanner(scanner2, nullptr, ReadWriteMode::READ_ONLY));
    int count2 = 0;
    RC rc = RC::SUCCESS;
    Record record;
    while (OB_SUCC(rc = scanner2.next(record))) {
      count2++;
    }

    ASSERT_EQ(insert_num, count2);
  }

  db2.reset();
  db.reset();
}

TEST(MvccTrxLog, wal2)
{
  /*
  创建一个数据库，建一些表，插入一些数据。执行一次sync，将所有buffer pool都落地，然后再创建一批表后所有表插入一部分数据。
  再等所有日志都落地，将文件都复制到另一个目录。此时buffer pool 应该都没落地。
  使用新的目录初始化一个新的数据库，然后检查数据是否一致。
  */
  filesystem::path test_directory("mvcc_trx_log_test");
  filesystem::remove_all(test_directory);
  filesystem::create_directory(test_directory);

  const char *dbname = "test_db";
  const char *dbname2 = "test_db2";
  filesystem::path db_path = test_directory / dbname;
  filesystem::path db_path2 = test_directory / dbname2;
  const char *trx_kit_name = "mvcc";

  auto db = make_unique<Db>();
  ASSERT_EQ(RC::SUCCESS, db->init(dbname, db_path.c_str(), trx_kit_name));

  const int table_num = 10;
  vector<string> table_names;
  for (int i = 0; i < table_num; i++) {
    table_names.push_back("table_" + to_string(i));
  }

  const int field_num = 10;
  vector<AttrInfoSqlNode> attr_infos;
  for (int i = 0; i < field_num; i++) {
    AttrInfoSqlNode attr_info;
    attr_info.name = string("field_") + to_string(i);
    attr_info.type = AttrType::INTS;
    attr_info.length = 4;
    attr_infos.push_back(attr_info);
  }

  for (const string &table_name : table_names) {
    ASSERT_EQ(RC::SUCCESS, db->create_table(table_name.c_str(), attr_infos.size(), attr_infos.data()));
    ASSERT_EQ(RC::SUCCESS, db->sync());
  }

  TrxKit &trx_kit = db->trx_kit();
  Trx *trx = trx_kit.create_trx(db->log_handler());
  const int insert_num = 10000;
  for (int i = 0; i < insert_num; i++) {
    ASSERT_NE(trx, nullptr);

    for (const string &table_name : table_names) {
      Table *table = db->find_table(table_name.c_str());
      ASSERT_NE(table, nullptr);
      
      Record record;

      vector<Value> values(field_num);
      for (Value &value : values) {
        value.set_int(i);
      }

      ASSERT_EQ(RC::SUCCESS, table->make_record(values.size(), values.data(), record));

      ASSERT_EQ(RC::SUCCESS, trx->insert_record(table, record));
    }

    ASSERT_EQ(RC::SUCCESS, trx->commit());
  }

  db->sync();

  const int table_num2 = 10;
  for (int i = table_num; i < table_num + table_num2; i++) {
    string table_name = "table_" + to_string(i);
    table_names.push_back(table_name);
    ASSERT_EQ(RC::SUCCESS, db->create_table(table_name.c_str(), attr_infos.size(), attr_infos.data()));
    ASSERT_EQ(RC::SUCCESS, db->sync());
  }

  const int insert_num2 = 10000;
  for (int i = 0; i < insert_num2; i++) {
    ASSERT_NE(trx, nullptr);

    for (const string &table_name : table_names) {
      Table *table = db->find_table(table_name.c_str());
      ASSERT_NE(table, nullptr);
      
      Record record;

      vector<Value> values(field_num);
      for (Value &value : values) {
        value.set_int(i);
      }

      ASSERT_EQ(RC::SUCCESS, table->make_record(values.size(), values.data(), record));

      ASSERT_EQ(RC::SUCCESS, trx->insert_record(table, record));
    }

    ASSERT_EQ(RC::SUCCESS, trx->commit());
  }


  DiskLogHandler &log_handler = static_cast<DiskLogHandler &>(db->log_handler());
  LSN current_lsn = log_handler.current_lsn();
  ASSERT_NE(RC::SUCCESS, log_handler.wait_lsn(current_lsn));

  // copy all files from db to db2
  filesystem::copy(db_path, db_path2, filesystem::copy_options::recursive);

  auto db2 = make_unique<Db>();
  ASSERT_EQ(RC::SUCCESS, db2->init(dbname2, db_path2.c_str(), trx_kit_name));
  vector<string> table_names2;
  db2->all_tables(table_names2);
  ASSERT_EQ(table_names, table_names2);

  // count each table's record number and take a check
  for (const string &table_name : table_names2) {

    Table *table2 = db2->find_table(table_name.c_str());
    ASSERT_NE(table2, nullptr);

    RecordFileScanner scanner2;
    ASSERT_EQ(RC::SUCCESS, table2->get_record_scanner(scanner2, nullptr, ReadWriteMode::READ_ONLY));
    int count2 = 0;
    RC rc = RC::SUCCESS;
    Record record;
    while (OB_SUCC(rc = scanner2.next(record))) {
      count2++;
    }

    ASSERT_EQ(insert_num + insert_num2, count2);
  }

  db2.reset();
  db.reset();
}

TEST(MvccTrxLog, wal_rollback)
{
  /*
  创建一个数据库，建一些表，插入一些数据，其中一部分回滚。
  然后等所有日志都落地，将文件都复制到另一个目录。此时buffer pool 应该都没落地。
  使用新的目录初始化一个新的数据库，然后检查数据是否一致。
  */
  filesystem::path test_directory("mvcc_trx_log_test");
  filesystem::remove_all(test_directory);
  filesystem::create_directory(test_directory);

  const char *dbname = "test_db";
  const char *dbname2 = "test_db2";
  filesystem::path db_path = test_directory / dbname;
  filesystem::path db_path2 = test_directory / dbname2;
  const char *trx_kit_name = "mvcc";

  auto db = make_unique<Db>();
  ASSERT_EQ(RC::SUCCESS, db->init(dbname, db_path.c_str(), trx_kit_name));

  const int table_num = 10;
  vector<string> table_names;
  for (int i = 0; i < table_num; i++) {
    table_names.push_back("table_" + to_string(i));
  }

  const int field_num = 10;
  vector<AttrInfoSqlNode> attr_infos;
  for (int i = 0; i < field_num; i++) {
    AttrInfoSqlNode attr_info;
    attr_info.name = string("field_") + to_string(i);
    attr_info.type = AttrType::INTS;
    attr_info.length = 4;
    attr_infos.push_back(attr_info);
  }

  for (const string &table_name : table_names) {
    ASSERT_EQ(RC::SUCCESS, db->create_table(table_name.c_str(), attr_infos.size(), attr_infos.data()));
    ASSERT_EQ(RC::SUCCESS, db->sync());
  }

  TrxKit &trx_kit = db->trx_kit();
  Trx *trx = trx_kit.create_trx(db->log_handler());
  const int insert_num = 10000;
  for (int i = 0; i < insert_num; i++) {
    ASSERT_NE(trx, nullptr);

    for (const string &table_name : table_names) {
      Table *table = db->find_table(table_name.c_str());
      ASSERT_NE(table, nullptr);
      
      Record record;

      vector<Value> values(field_num);
      for (Value &value : values) {
        value.set_int(i);
      }

      ASSERT_EQ(RC::SUCCESS, table->make_record(values.size(), values.data(), record));

      ASSERT_EQ(RC::SUCCESS, trx->insert_record(table, record));
    }

    if (insert_num % 2 == 0) {
      ASSERT_EQ(RC::SUCCESS, trx->commit());
    } else {
      ASSERT_EQ(RC::SUCCESS, trx->rollback());
    }
  }

  DiskLogHandler &log_handler = static_cast<DiskLogHandler &>(db->log_handler());
  LSN current_lsn = log_handler.current_lsn();
  ASSERT_NE(RC::SUCCESS, log_handler.wait_lsn(current_lsn));

  // copy all files from db to db2
  filesystem::copy(db_path, db_path2, filesystem::copy_options::recursive);

  auto db2 = make_unique<Db>();
  ASSERT_EQ(RC::SUCCESS, db2->init(dbname2, db_path2.c_str(), trx_kit_name));
  vector<string> table_names2;
  db2->all_tables(table_names2);
  ASSERT_EQ(table_names, table_names2);

  // count each table's record number and take a check
  for (const string &table_name : table_names2) {

    Table *table2 = db2->find_table(table_name.c_str());
    ASSERT_NE(table2, nullptr);

    RecordFileScanner scanner2;
    ASSERT_EQ(RC::SUCCESS, table2->get_record_scanner(scanner2, nullptr, ReadWriteMode::READ_ONLY));
    int count2 = 0;
    Record record;
    RC rc = RC::SUCCESS;
    while (OB_SUCC(rc = scanner2.next(record))) {
      count2++;
    }

    ASSERT_EQ(insert_num / 2, count2);
  }

  db2.reset();
  db.reset();
}


TEST(MvccTrxLog, wal_rollback_abnormal)
{
  /*
  创建一个数据库，建一些表，插入一些数据，其中一部分不提交也不回滚(这部分事务应该在恢复时回滚掉)。
  然后等所有日志都落地，将文件都复制到另一个目录。此时buffer pool 应该都没落地。
  使用新的目录初始化一个新的数据库，然后检查数据是否一致。
  */
  filesystem::path test_directory("mvcc_trx_log_test");
  filesystem::remove_all(test_directory);
  filesystem::create_directory(test_directory);

  const char *dbname = "test_db";
  const char *dbname2 = "test_db2";
  filesystem::path db_path = test_directory / dbname;
  filesystem::path db_path2 = test_directory / dbname2;
  const char *trx_kit_name = "mvcc";

  auto db = make_unique<Db>();
  ASSERT_EQ(RC::SUCCESS, db->init(dbname, db_path.c_str(), trx_kit_name));

  const int table_num = 10;
  vector<string> table_names;
  for (int i = 0; i < table_num; i++) {
    table_names.push_back("table_" + to_string(i));
  }

  const int field_num = 10;
  vector<AttrInfoSqlNode> attr_infos;
  for (int i = 0; i < field_num; i++) {
    AttrInfoSqlNode attr_info;
    attr_info.name = string("field_") + to_string(i);
    attr_info.type = AttrType::INTS;
    attr_info.length = 4;
    attr_infos.push_back(attr_info);
  }

  for (const string &table_name : table_names) {
    ASSERT_EQ(RC::SUCCESS, db->create_table(table_name.c_str(), attr_infos.size(), attr_infos.data()));
    ASSERT_EQ(RC::SUCCESS, db->sync());
  }

  TrxKit &trx_kit = db->trx_kit();
  const int insert_num = 10000;
  for (int i = 0; i < insert_num; i++) {
    Trx *trx = trx_kit.create_trx(db->log_handler());
    ASSERT_NE(trx, nullptr);

    for (const string &table_name : table_names) {
      Table *table = db->find_table(table_name.c_str());
      ASSERT_NE(table, nullptr);
      
      Record record;

      vector<Value> values(field_num);
      for (Value &value : values) {
        value.set_int(i);
      }

      ASSERT_EQ(RC::SUCCESS, table->make_record(values.size(), values.data(), record));

      ASSERT_EQ(RC::SUCCESS, trx->insert_record(table, record));
    }

    if (insert_num % 2 == 0) {
      ASSERT_EQ(RC::SUCCESS, trx->commit());
    }
    trx_kit.destroy_trx(trx);
  }

  DiskLogHandler &log_handler = static_cast<DiskLogHandler &>(db->log_handler());
  LSN current_lsn = log_handler.current_lsn();
  ASSERT_NE(RC::SUCCESS, log_handler.wait_lsn(current_lsn));

  // copy all files from db to db2
  filesystem::copy(db_path, db_path2, filesystem::copy_options::recursive);

  auto db2 = make_unique<Db>();
  ASSERT_EQ(RC::SUCCESS, db2->init(dbname2, db_path2.c_str(), trx_kit_name));
  vector<string> table_names2;
  db2->all_tables(table_names2);
  ASSERT_EQ(table_names, table_names2);

  // count each table's record number and take a check
  for (const string &table_name : table_names2) {

    Table *table2 = db2->find_table(table_name.c_str());
    ASSERT_NE(table2, nullptr);

    RecordFileScanner scanner2;
    ASSERT_EQ(RC::SUCCESS, table2->get_record_scanner(scanner2, nullptr, ReadWriteMode::READ_ONLY));
    int count2 = 0;
    Record record;
    RC rc = RC::SUCCESS;
    while (OB_SUCC(rc = scanner2.next(record))) {
      count2++;
    }

    ASSERT_EQ(insert_num / 2, count2);
  }

  db2.reset();
  db.reset();
}

int main(int argc, char **argv)
{
  testing::InitGoogleTest(&argc, argv);
  filesystem::path log_filename = filesystem::path(argv[0]).filename();
  LoggerFactory::init_default(log_filename.string() + ".log", LOG_LEVEL_TRACE);
  return RUN_ALL_TESTS();
}