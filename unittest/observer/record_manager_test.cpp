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
#include <filesystem>
#include <utility>

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

TEST(RecordPageHandler, test_record_page_handler)
{
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

  const int          record_size        = 8;
  RecordPageHandler *record_page_handle = new RowRecordPageHandler();
  rc = record_page_handle->init_empty_page(*bp, log_handler, frame->page_num(), record_size, nullptr);
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

  char buf[record_size];
  RID  rid;
  rid.page_num = 100;
  rid.slot_num = 100;
  rc           = record_page_handle->insert_record(buf, &rid);
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
    rc           = record_page_handle->insert_record(buf, &rid);
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
    rc           = record_page_handle->delete_record(&rid);
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

  record_page_handle->cleanup();
  bpm->close_file(record_manager_file);
  delete bpm;
  delete record_page_handle;
}

TEST(RecordFileScanner, test_record_file_iterator)
{
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

  RecordFileHandler file_handler(StorageFormat::ROW_FORMAT);
  rc = file_handler.init(*bp, log_handler, nullptr);
  ASSERT_EQ(rc, RC::SUCCESS);

  VacuousTrx        trx;
  RecordFileScanner file_scanner;
  rc = file_scanner.open_scan(
      nullptr /*table*/, *bp, &trx, log_handler, ReadWriteMode::READ_ONLY, nullptr /*condition_filter*/);
  ASSERT_EQ(rc, RC::SUCCESS);

  int    count = 0;
  Record record;
  while (OB_SUCC(rc = file_scanner.next(record))) {
    count++;
  }
  if (rc == RC::RECORD_EOF) {
    rc = RC::SUCCESS;
  }
  file_scanner.close_scan();
  ASSERT_EQ(count, 0);

  const int        record_insert_num = 1000;
  char             record_data[20];
  std::vector<RID> rids;
  for (int i = 0; i < record_insert_num; i++) {
    RID rid;
    rc = file_handler.insert_record(record_data, sizeof(record_data), &rid);
    ASSERT_EQ(rc, RC::SUCCESS);
    rids.push_back(rid);
  }

  rc = file_scanner.open_scan(
      nullptr /*table*/, *bp, &trx, log_handler, ReadWriteMode::READ_ONLY, nullptr /*condition_filter*/);
  ASSERT_EQ(rc, RC::SUCCESS);

  count = 0;
  while (OB_SUCC(rc = file_scanner.next(record))) {
    count++;
  }
  ASSERT_EQ(RC::RECORD_EOF, rc);

  file_scanner.close_scan();
  ASSERT_EQ(count, rids.size());

  for (int i = 0; i < record_insert_num; i += 2) {
    rc = file_handler.delete_record(&rids[i]);
    ASSERT_EQ(rc, RC::SUCCESS);
  }

  rc = file_scanner.open_scan(
      nullptr /*table*/, *bp, &trx, log_handler, ReadWriteMode::READ_ONLY, nullptr /*condition_filter*/);
  ASSERT_EQ(rc, RC::SUCCESS);

  count = 0;
  while (OB_SUCC(rc = file_scanner.next(record))) {
    count++;
  }
  ASSERT_EQ(RC::RECORD_EOF, rc);

  file_scanner.close_scan();
  ASSERT_EQ(count, rids.size() / 2);

  bpm->close_file(record_manager_file);
  delete bpm;
}

TEST(RecordManager, durability)
{
  /*
   * 测试场景：
   * 1. 创建一个文件，插入一些记录
   * 2. 随机进行插入、更新、删除操作
   * 3. 重启数据库，检查记录是否恢复
   */
  filesystem::path directory("record_manager_durability");
  filesystem::remove_all(directory);
  ASSERT_TRUE(filesystem::create_directories(directory));

  filesystem::path record_manager_file = directory / "record_manager.bp";

  BufferPoolManager                    bpm;
  unique_ptr<VacuousDoubleWriteBuffer> double_write_buffer = make_unique<VacuousDoubleWriteBuffer>();
  ASSERT_EQ(bpm.init(std::move(double_write_buffer)), RC::SUCCESS);

  DiskLogHandler        log_handler;
  IntegratedLogReplayer log_replayer(bpm);
  ASSERT_EQ(log_handler.init(directory.c_str()), RC::SUCCESS);
  ASSERT_EQ(log_handler.replay(log_replayer, 0), RC::SUCCESS);
  ASSERT_EQ(log_handler.start(), RC::SUCCESS);

  DiskBufferPool *buffer_pool = nullptr;
  ASSERT_EQ(bpm.create_file(record_manager_file.c_str()), RC::SUCCESS);
  ASSERT_EQ(bpm.open_file(log_handler, record_manager_file.c_str(), buffer_pool), RC::SUCCESS);
  ASSERT_NE(buffer_pool, nullptr);

  RecordFileHandler record_file_handler(StorageFormat::ROW_FORMAT);
  ASSERT_EQ(record_file_handler.init(*buffer_pool, log_handler, nullptr), RC::SUCCESS);

  const int  record_size              = 100;
  const char record_data[record_size] = "hello, world!";
  const int  insert_record_num        = 1000;

  unordered_map<RID, string, RIDHash> record_map;
  mutex                               record_map_lock;
  for (int i = 0; i < insert_record_num; i++) {
    RID rid;
    ASSERT_EQ(record_file_handler.insert_record(record_data, record_size, &rid), RC::SUCCESS);
    record_map.emplace(rid, string(record_data, record_size));
  }

  IntegerGenerator record_suffix_random(0, 1000000000);

  // 希望可以并发的进行插入更新和删除
  // 但是为了记录每次操作的结果，所以要加锁，加锁导致最后效果相当于没有并行
  ThreadPoolExecutor executor;
  ASSERT_EQ(executor.init("RecordManagerTest", 4, 4, 60 * 1000), 0);

  IntegerGenerator operation_random(0, 2);
  const int        random_operation_num = 1000;
  for (int i = 0; i < random_operation_num; i++) {
    auto task =
        [&operation_random, &record_suffix_random, &record_file_handler, &record_map, &record_map_lock, record_data]() {
          int operation_type = operation_random.next();
          switch (operation_type) {
            case 0: {  // insert
              RID rid;
              record_map_lock.lock();
              ASSERT_EQ(record_file_handler.insert_record(record_data, record_size, &rid), RC::SUCCESS);
              record_map.emplace(rid, string(record_data));
              record_map_lock.unlock();
              break;
            }
            case 1: {  // update

              int    suffix     = record_suffix_random.next();
              string new_record = string(record_data) + to_string(suffix);

              record_map_lock.lock();
              IntegerGenerator record_random(0, record_map.size() - 1);
              auto             iter = record_map.begin();
              advance(iter, record_random.next());
              RID rid = iter->first;
              ASSERT_EQ(record_file_handler.visit_record(rid,
                            [&new_record](Record &record) {
                              memcpy(record.data(), new_record.c_str(), new_record.size());
                              return true;
                            }),
                  RC::SUCCESS);

              record_map[rid] = new_record;
              record_map_lock.unlock();

              break;
            }

            case 2: {  // delete
              record_map_lock.lock();
              IntegerGenerator record_random(0, record_map.size() - 1);
              int              index = record_random.next();
              auto             iter  = record_map.begin();
              advance(iter, index);
              RID rid = iter->first;
              ASSERT_EQ(record_file_handler.delete_record(&rid), RC::SUCCESS);
              record_map.erase(rid);
              record_map_lock.unlock();
              break;
            }

            default: {
              ASSERT_TRUE(false);
            }
          }
        };

    ASSERT_EQ(executor.execute(task), 0);
  }

  ASSERT_EQ(executor.shutdown(), 0);
  ASSERT_EQ(executor.await_termination(), 0);

  // 把文件复制出来
  filesystem::path record_manager_file_copy = directory / "record_manager_copy.bp";
  filesystem::copy_file(record_manager_file, record_manager_file_copy);
  // 删掉旧文件
  bpm.close_file(record_manager_file.c_str());
  filesystem::remove(record_manager_file);
  // 停掉log_handler
  ASSERT_EQ(log_handler.stop(), RC::SUCCESS);
  ASSERT_EQ(log_handler.await_termination(), RC::SUCCESS);

  // 重新创建资源并尝试从日志中恢复数据，然后校验数据
  DiskLogHandler    log_handler2;
  BufferPoolManager bpm2;
  ASSERT_EQ(RC::SUCCESS, bpm2.init(make_unique<VacuousDoubleWriteBuffer>()));
  DiskBufferPool *buffer_pool2 = nullptr;
  filesystem::copy(record_manager_file_copy, record_manager_file);
  ASSERT_EQ(bpm2.open_file(log_handler2, record_manager_file.c_str(), buffer_pool2), RC::SUCCESS);
  ASSERT_NE(buffer_pool2, nullptr);

  IntegratedLogReplayer log_replayer2(bpm2);
  ASSERT_EQ(log_handler2.init(directory.c_str()), RC::SUCCESS);
  ASSERT_EQ(log_handler2.replay(log_replayer2, 0), RC::SUCCESS);
  ASSERT_EQ(log_handler2.start(), RC::SUCCESS);

  RecordFileHandler record_file_handler2(StorageFormat::ROW_FORMAT);
  ASSERT_EQ(record_file_handler2.init(*buffer_pool2, log_handler2, nullptr), RC::SUCCESS);
  for (const auto &[rid, record] : record_map) {
    Record record_data;
    ASSERT_EQ(record_file_handler2.get_record(rid, record_data), RC::SUCCESS);
    ASSERT_EQ(memcmp(record_data.data(), record.c_str(), record.size()), 0);
  }

  ASSERT_EQ(log_handler2.stop(), RC::SUCCESS);
  ASSERT_EQ(log_handler2.await_termination(), RC::SUCCESS);
  bpm2.close_file(record_manager_file.c_str());
}

int main(int argc, char **argv)
{
  testing::InitGoogleTest(&argc, argv);
  filesystem::path log_filename = filesystem::path(argv[0]).filename();
  LoggerFactory::init_default(log_filename.string() + ".log", LOG_LEVEL_TRACE);
  return RUN_ALL_TESTS();
}
