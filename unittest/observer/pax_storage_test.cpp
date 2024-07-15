/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include <string.h>
#include <sstream>
#include <filesystem>
#include <utility>

#define protected public
#define private public
#include "storage/table/table.h"
#undef protected
#undef private

#include "storage/buffer/disk_buffer_pool.h"
#include "storage/record/record_manager.h"
#include "storage/trx/vacuous_trx.h"
#include "storage/clog/vacuous_log_handler.h"
#include "storage/clog/disk_log_handler.h"
#include "storage/buffer/double_write_buffer.h"
#include "common/math/integer_generator.h"
#include "common/thread/thread_pool_executor.h"
#include "storage/clog/integrated_log_replayer.h"
#include "gtest/gtest.h"

using namespace std;
using namespace common;

class PaxRecordFileScannerWithParam : public testing::TestWithParam<int>
{};

TEST_P(PaxRecordFileScannerWithParam, test_file_iterator)
{
  int               record_insert_num = GetParam();
  VacuousLogHandler log_handler;

  const char *record_manager_file = "record_manager.bp";
  filesystem::remove(record_manager_file);

  BufferPoolManager *bpm = new BufferPoolManager();
  ASSERT_EQ(RC::SUCCESS, bpm->init(make_unique<VacuousDoubleWriteBuffer>()));
  DiskBufferPool *bp = nullptr;
  RC              rc = bpm->create_file(record_manager_file);
  ASSERT_EQ(rc, RC::SUCCESS);

  rc = bpm->open_file(log_handler, record_manager_file, bp);
  ASSERT_EQ(rc, RC::SUCCESS);

  TableMeta table_meta;
  table_meta.fields_.resize(2);
  table_meta.fields_[0].attr_type_ = AttrType::INTS;
  table_meta.fields_[0].attr_len_  = 4;
  table_meta.fields_[0].field_id_ = 0;
  table_meta.fields_[1].attr_type_ = AttrType::INTS;
  table_meta.fields_[1].attr_len_  = 4;
  table_meta.fields_[1].field_id_ = 1;

  RecordFileHandler file_handler(StorageFormat::PAX_FORMAT);
  rc = file_handler.init(*bp, log_handler, &table_meta);
  ASSERT_EQ(rc, RC::SUCCESS);

  VacuousTrx        trx;
  ChunkFileScanner chunk_scanner;
  RecordFileScanner record_scanner;
  Table             table;
  table.table_meta_.storage_format_ = StorageFormat::PAX_FORMAT;
  // no record
  // record iterator
  rc = record_scanner.open_scan(&table, *bp, &trx, log_handler, ReadWriteMode::READ_ONLY, nullptr /*condition_filter*/);
  ASSERT_EQ(rc, RC::SUCCESS);

  int    count = 0;
  Record record;
  while (OB_SUCC(rc = record_scanner.next(record))) {
    count++;
  }
  if (rc == RC::RECORD_EOF) {
    rc = RC::SUCCESS;
  }
  record_scanner.close_scan();
  ASSERT_EQ(count, 0);

  // chunk iterator
  rc = chunk_scanner.open_scan_chunk(&table, *bp, log_handler, ReadWriteMode::READ_ONLY);
  ASSERT_EQ(rc, RC::SUCCESS);
  Chunk     chunk;
  FieldMeta fm;
  fm.init("col1", AttrType::INTS, 0, 4, true, 0);
  auto col1 = std::make_unique<Column>(fm, 2048);
  chunk.add_column(std::move(col1), 0);
  count = 0;
  while (OB_SUCC(rc = chunk_scanner.next_chunk(chunk))) {
    count += chunk.rows();
    chunk.reset_data();
  }
  ASSERT_EQ(rc, RC::RECORD_EOF);
  ASSERT_EQ(count, 0);

  // insert some records
  char             record_data[8];
  std::vector<RID> rids;
  for (int i = 0; i < record_insert_num; i++) {
    RID rid;
    rc = file_handler.insert_record(record_data, sizeof(record_data), &rid);
    ASSERT_EQ(rc, RC::SUCCESS);
    rids.push_back(rid);
  }

  // record iterator
  rc = record_scanner.open_scan(&table, *bp, &trx, log_handler, ReadWriteMode::READ_ONLY, nullptr /*condition_filter*/);
  ASSERT_EQ(rc, RC::SUCCESS);

  count = 0;
  while (OB_SUCC(rc = record_scanner.next(record))) {
    count++;
  }
  ASSERT_EQ(RC::RECORD_EOF, rc);

  record_scanner.close_scan();
  ASSERT_EQ(count, rids.size());

  // chunk iterator
  rc = chunk_scanner.open_scan_chunk(&table, *bp, log_handler, ReadWriteMode::READ_ONLY);
  ASSERT_EQ(rc, RC::SUCCESS);
  chunk.reset_data();
  count = 0;
  while (OB_SUCC(rc = chunk_scanner.next_chunk(chunk))) {
    count += chunk.rows();
    chunk.reset_data();
  }
  ASSERT_EQ(rc, RC::RECORD_EOF);
  ASSERT_EQ(count, record_insert_num);

  // delete some records
  for (int i = 0; i < record_insert_num; i += 2) {
    rc = file_handler.delete_record(&rids[i]);
    ASSERT_EQ(rc, RC::SUCCESS);
  }

  // record iterator
  rc = record_scanner.open_scan(&table, *bp, &trx, log_handler, ReadWriteMode::READ_ONLY, nullptr /*condition_filter*/);
  ASSERT_EQ(rc, RC::SUCCESS);

  count = 0;
  while (OB_SUCC(rc = record_scanner.next(record))) {
    count++;
  }
  ASSERT_EQ(RC::RECORD_EOF, rc);

  record_scanner.close_scan();
  ASSERT_EQ(count, rids.size() / 2);

  // chunk iterator
  rc = chunk_scanner.open_scan_chunk(&table, *bp, log_handler, ReadWriteMode::READ_ONLY);
  ASSERT_EQ(rc, RC::SUCCESS);
  chunk.reset_data();
  count = 0;
  while (OB_SUCC(rc = chunk_scanner.next_chunk(chunk))) {
    count += chunk.rows();
    chunk.reset_data();
  }
  ASSERT_EQ(rc, RC::RECORD_EOF);
  ASSERT_EQ(count, rids.size() / 2);

  bpm->close_file(record_manager_file);
  delete bpm;
}

class PaxPageHandlerTestWithParam : public testing::TestWithParam<int>
{};

TEST_P(PaxPageHandlerTestWithParam, PaxPageHandler)
{
  int               record_num = GetParam();
  VacuousLogHandler log_handler;

  const char *record_manager_file = "record_manager.bp";
  ::remove(record_manager_file);

  BufferPoolManager *bpm = new BufferPoolManager();
  ASSERT_EQ(RC::SUCCESS, bpm->init(make_unique<VacuousDoubleWriteBuffer>()));
  DiskBufferPool *bp = nullptr;
  RC              rc = bpm->create_file(record_manager_file);
  ASSERT_EQ(rc, RC::SUCCESS);

  rc = bpm->open_file(log_handler, record_manager_file, bp);
  ASSERT_EQ(rc, RC::SUCCESS);

  Frame *frame = nullptr;
  rc           = bp->allocate_page(&frame);
  ASSERT_EQ(rc, RC::SUCCESS);

  const int          record_size        = 19;  // 4 + 4 + 4 + 7
  RecordPageHandler *record_page_handle = new PaxRecordPageHandler();
  TableMeta          table_meta;
  table_meta.fields_.resize(4);
  table_meta.fields_[0].attr_type_ = AttrType::INTS;
  table_meta.fields_[0].attr_len_  = 4;
  table_meta.fields_[0].field_id_ = 0;
  table_meta.fields_[1].attr_type_ = AttrType::FLOATS;
  table_meta.fields_[1].attr_len_  = 4;
  table_meta.fields_[1].field_id_ = 1;
  table_meta.fields_[2].attr_type_ = AttrType::CHARS;
  table_meta.fields_[2].attr_len_  = 4;
  table_meta.fields_[2].field_id_ = 2;
  table_meta.fields_[3].attr_type_ = AttrType::CHARS;
  table_meta.fields_[3].attr_len_  = 7;
  table_meta.fields_[3].field_id_ = 3;

  rc = record_page_handle->init_empty_page(*bp, log_handler, frame->page_num(), record_size, &table_meta);
  ASSERT_EQ(rc, RC::SUCCESS);

  RecordPageIterator iterator;
  iterator.init(record_page_handle);

  int    count = 0;
  Record record;
  while (iterator.has_next()) {
    count++;
    rc = iterator.next(record);
    ASSERT_EQ(rc, RC::SUCCESS);
  }
  ASSERT_EQ(count, 0);

  char  buf[record_size];
  int   int_base   = 122;
  float float_base = 1.45;
  memcpy(buf + 8, "12345678910", 4 + 7);
  RID rid;
  for (int i = 0; i < record_num; i++) {
    int   int_val   = i + int_base;
    float float_val = i + float_base;
    memcpy(buf, &int_val, sizeof(int));
    memcpy(buf + 4, &float_val, sizeof(float));
    rc = record_page_handle->insert_record(buf, &rid);
    ASSERT_EQ(rc, RC::SUCCESS);
  }

  count = 0;
  iterator.init(record_page_handle);
  while (iterator.has_next()) {
    int   int_val   = count + int_base;
    float float_val = count + float_base;
    rc              = iterator.next(record);
    ASSERT_EQ(memcmp(record.data(), &int_val, sizeof(int)), 0);
    ASSERT_EQ(memcmp(record.data() + 4, &float_val, sizeof(float)), 0);
    ASSERT_EQ(memcmp(record.data() + 8, "12345678910", 4 + 7), 0);
    ASSERT_EQ(rc, RC::SUCCESS);
    count++;
  }
  ASSERT_EQ(count, record_num);

  Chunk     chunk1;
  FieldMeta fm1, fm2, fm3, fm4;
  fm1.init("col1", AttrType::INTS, 0, 4, true, 0);
  fm2.init("col2", AttrType::FLOATS, 4, 4, true, 1);
  fm3.init("col3", AttrType::CHARS, 8, 4, true, 2);
  fm4.init("col4", AttrType::CHARS, 12, 7, true, 3);
  auto col_1 = std::make_unique<Column>(fm1, 2048);
  chunk1.add_column(std::move(col_1), 0);
  auto col_2 = std::make_unique<Column>(fm2, 2048);
  chunk1.add_column(std::move(col_2), 1);
  auto col_3 = std::make_unique<Column>(fm3, 2048);
  chunk1.add_column(std::move(col_3), 2);
  auto col_4 = std::make_unique<Column>(fm4, 2048);
  chunk1.add_column(std::move(col_4), 3);
  rc = record_page_handle->get_chunk(chunk1);
  ASSERT_EQ(rc, RC::SUCCESS);
  ASSERT_EQ(chunk1.rows(), record_num);
  for (int i = 0; i < record_num; i++) {
    int   int_val   = i + int_base;
    float float_val = i + float_base;
    ASSERT_EQ(chunk1.get_value(0, i).get_int(), int_val);
    ASSERT_EQ(chunk1.get_value(1, i).get_float(), float_val);
    ASSERT_STREQ(chunk1.get_value(2, i).get_string().c_str(), "1234");
    ASSERT_STREQ(chunk1.get_value(3, i).get_string().c_str(), "5678910");
  }

  Chunk     chunk2;
  FieldMeta fm2_1;
  fm2_1.init("col2", AttrType::FLOATS, 4, 4, true, 1);
  auto col_2_1 = std::make_unique<Column>(fm2_1, 2048);
  chunk2.add_column(std::move(col_2_1), 1);
  rc = record_page_handle->get_chunk(chunk2);
  ASSERT_EQ(rc, RC::SUCCESS);
  ASSERT_EQ(chunk2.rows(), record_num);
  for (int i = 0; i < record_num; i++) {
    float float_val = i + float_base;
    ASSERT_FLOAT_EQ(chunk2.get_value(0, i).get_float(), float_val);
  }

  // delete record
  IntegerGenerator generator(0, record_num - 1);
  int delete_num = generator.next();
  std::set<int> delete_slots;
  for (int i = 0; i < delete_num; i++) {

    int slot_num = 0;
    while (true) {
      slot_num = generator.next();
      if (delete_slots.find(slot_num) == delete_slots.end()) break;
    }
    RID del_rid(1, slot_num);
    rc = record_page_handle->delete_record(&del_rid);
    delete_slots.insert(slot_num);
    ASSERT_EQ(rc, RC::SUCCESS);
  }

  // get chunk
  chunk1.reset_data();
  record_page_handle->get_chunk(chunk1);
  ASSERT_EQ(chunk1.rows(), record_num - delete_num);

  int col1_expected = (int_base + 0 + int_base + record_num - 1) * record_num /2;
  int col1_actual   = 0;
  for (int i = 0; i < chunk1.rows(); i++) {
    col1_actual += chunk1.get_value(0, i).get_int();
  }
  for (auto it = delete_slots.begin(); it != delete_slots.end(); ++it) {
    col1_actual += *it + int_base;
  }
  // check sum(col1)
  ASSERT_EQ(col1_actual, col1_expected);
  for (auto it = delete_slots.begin(); it != delete_slots.end(); ++it) {
    RID get_rid(1, *it);
    ASSERT_EQ(record_page_handle->get_record(get_rid, record), RC::RECORD_NOT_EXIST);
  }

  rc = record_page_handle->cleanup();
  ASSERT_EQ(rc, RC::SUCCESS);
  delete record_page_handle;
  bpm->close_file(record_manager_file);
  delete bpm;
}

INSTANTIATE_TEST_SUITE_P(PaxFileScannerTests, PaxRecordFileScannerWithParam, testing::Values(1, 10, 100, 1000, 2000, 10000));

INSTANTIATE_TEST_SUITE_P(PaxPageTests, PaxPageHandlerTestWithParam, testing::Values(1, 10, 100, 337));

int main(int argc, char **argv)
{
  testing::InitGoogleTest(&argc, argv);
  filesystem::path log_filename = filesystem::path(argv[0]).filename();
  LoggerFactory::init_default(log_filename.string() + ".log", LOG_LEVEL_TRACE);
  return RUN_ALL_TESTS();
}
