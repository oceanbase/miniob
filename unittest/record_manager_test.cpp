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
// Created by wangyunlai.wyl on 2022
//

#include <string.h>
#include <sstream>

#include "gtest/gtest.h"
#include "storage/buffer/disk_buffer_pool.h"
#include "storage/record/record_manager.h"
#include "storage/trx/vacuous_trx.h"

using namespace common;

TEST(test_record_page_handler, test_record_page_handler)
{
  const char *record_manager_file = "record_manager.bp";
  ::remove(record_manager_file);

  BufferPoolManager *bpm = new BufferPoolManager();
  DiskBufferPool *bp = nullptr;
  RC rc = bpm->create_file(record_manager_file);
  ASSERT_EQ(rc, RC::SUCCESS);
  
  rc = bpm->open_file(record_manager_file, bp);
  ASSERT_EQ(rc, RC::SUCCESS);

  Frame *frame = nullptr;
  rc = bp->allocate_page(&frame);
  ASSERT_EQ(rc, RC::SUCCESS);

  const int record_size = 8;
  RecordPageHandler record_page_handle;
  rc = record_page_handle.init_empty_page(*bp, frame->page_num(), record_size);
  ASSERT_EQ(rc, RC::SUCCESS);

  RecordPageIterator iterator;
  iterator.init(record_page_handle);

  int count = 0;
  Record record;
  while (iterator.has_next()) {
    count++;
    rc = iterator.next(record);
    ASSERT_EQ(rc, RC::SUCCESS);
  }
  ASSERT_EQ(count, 0);

  char buf[record_size];
  RID rid;
  rid.page_num = 100;
  rid.slot_num = 100;
  rc = record_page_handle.insert_record(buf, &rid);
  ASSERT_EQ(rc, RC::SUCCESS);

  count = 0;
  iterator.init(record_page_handle);
  while (iterator.has_next()) {
    count++;
    rc = iterator.next(record);
    ASSERT_EQ(rc, RC::SUCCESS);
  }
  ASSERT_EQ(count, 1);

  for (int i = 0; i < 10; i++) {
    rid.page_num = i;
    rid.slot_num = i;
    rc = record_page_handle.insert_record(buf, &rid);
    ASSERT_EQ(rc, RC::SUCCESS);
  }

  count = 0;
  iterator.init(record_page_handle);
  while (iterator.has_next()) {
    count++;
    rc = iterator.next(record);
    ASSERT_EQ(rc, RC::SUCCESS);
  }
  ASSERT_EQ(count, 11);

  for (int i = 0; i < 5; i++) {
    rid.page_num = i * 2;
    rid.slot_num = i * 2;
    rc = record_page_handle.delete_record(&rid);
    ASSERT_EQ(rc, RC::SUCCESS);
  }

  count = 0;
  iterator.init(record_page_handle);
  while (iterator.has_next()) {
    count++;
    rc = iterator.next(record);
    ASSERT_EQ(rc, RC::SUCCESS);
  }
  ASSERT_EQ(count, 6);

  record_page_handle.cleanup();
  bpm->close_file(record_manager_file);
  delete bpm;
}

TEST(test_record_page_handler, test_record_file_iterator)
{
  const char *record_manager_file = "record_manager.bp";
  ::remove(record_manager_file);

  BufferPoolManager *bpm = new BufferPoolManager();
  DiskBufferPool *bp = nullptr;
  RC rc = bpm->create_file(record_manager_file);
  ASSERT_EQ(rc, RC::SUCCESS);
  
  rc = bpm->open_file(record_manager_file, bp);
  ASSERT_EQ(rc, RC::SUCCESS);

  RecordFileHandler file_handler;
  rc = file_handler.init(bp);
  ASSERT_EQ(rc, RC::SUCCESS);

  VacuousTrx trx;
  RecordFileScanner file_scanner;
  rc = file_scanner.open_scan(nullptr/*table*/, *bp, &trx, true/*readonly*/, nullptr/*condition_filter*/);
  ASSERT_EQ(rc, RC::SUCCESS);

  int count = 0;
  Record record;
  while (file_scanner.has_next()) {
    rc = file_scanner.next(record);
    ASSERT_EQ(rc, RC::SUCCESS);
    count++;
  }
  file_scanner.close_scan();
  ASSERT_EQ(count, 0);
  
  const int record_insert_num = 1000;
  char record_data[20];
  std::vector<RID> rids;
  for (int i = 0; i < record_insert_num; i++) {
    RID rid;
    rc = file_handler.insert_record(record_data, sizeof(record_data), &rid);
    ASSERT_EQ(rc, RC::SUCCESS);
    rids.push_back(rid);
  }

  rc = file_scanner.open_scan(nullptr/*table*/, *bp, &trx, true/*readonly*/, nullptr/*condition_filter*/);
  ASSERT_EQ(rc, RC::SUCCESS);

  count = 0;
  while (file_scanner.has_next()) {
    rc = file_scanner.next(record);
    ASSERT_EQ(rc, RC::SUCCESS);
    count++;
  }
  file_scanner.close_scan();
  ASSERT_EQ(count, rids.size());
  
  for (int i = 0; i < record_insert_num; i += 2) {
    rc = file_handler.delete_record(&rids[i]);
    ASSERT_EQ(rc, RC::SUCCESS);
  }

  rc = file_scanner.open_scan(nullptr/*table*/, *bp, &trx, true/*readonly*/, nullptr/*condition_filter*/);
  ASSERT_EQ(rc, RC::SUCCESS);

  count = 0;
  while (file_scanner.has_next()) {
    rc = file_scanner.next(record);
    ASSERT_EQ(rc, RC::SUCCESS);
    count++;
  }
  file_scanner.close_scan();
  ASSERT_EQ(count, rids.size() / 2);
  
  bpm->close_file(record_manager_file);
  delete bpm;
}

int main(int argc, char **argv)
{
  // 分析gtest程序的命令行参数
  testing::InitGoogleTest(&argc, argv);

  // 调用RUN_ALL_TESTS()运行所有测试用例
  // main函数返回RUN_ALL_TESTS()的运行结果
  return RUN_ALL_TESTS();
}
