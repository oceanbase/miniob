/* Copyright (c) 2021-2022 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by wangyunlai on 2024/02/01
//

#include <filesystem>

#include "gtest/gtest.h"
#include "common/log/log.h"
#include "storage/buffer/disk_buffer_pool.h"
#include "storage/clog/vacuous_log_handler.h"
#include "storage/buffer/double_write_buffer.h"

using namespace std;
using namespace common;

int buffer_pool_page_count(DiskBufferPool *buffer_pool)
{
  int                count = 0;
  BufferPoolIterator iterator;
  iterator.init(*buffer_pool, 1);
  while (iterator.has_next()) {
    iterator.next();
    count++;
  }
  return count;
}

TEST(DiskBufferPool, allocate_dispose)
{
  /*
  1. 创建buffer pool manager
  2. 创建buffer pool log handler
  3. 创建disk buffer pool
  4. 分配100个页面
  5. 分配100个页面并释放50个页面
  6. 统计buffer pool的总页面
  7. 重启一下检查页面总个数
  */

  filesystem::path directory("buffer_pool");
  filesystem::remove_all(directory);
  filesystem::create_directories(directory);

  filesystem::path buffer_pool_filename = directory / "buffer_pool.bp";

  // 1. 创建buffer pool manager
  BufferPoolManager buffer_pool_manager;
  ASSERT_EQ(RC::SUCCESS, buffer_pool_manager.init(make_unique<VacuousDoubleWriteBuffer>()));

  // 2. 创建buffer pool log handler
  VacuousLogHandler log_handler;

  // 3. 创建disk buffer pool
  ASSERT_EQ(RC::SUCCESS, buffer_pool_manager.create_file(buffer_pool_filename.c_str()));
  DiskBufferPool *buffer_pool = nullptr;
  ASSERT_EQ(RC::SUCCESS, buffer_pool_manager.open_file(log_handler, buffer_pool_filename.c_str(), buffer_pool));
  ASSERT_NE(buffer_pool, nullptr);

  // 4. 分配100个页面
  const int allocate_page_num = 100;
  for (int i = 0; i < allocate_page_num; ++i) {
    Frame *frame = nullptr;
    ASSERT_EQ(RC::SUCCESS, buffer_pool->allocate_page(&frame));
    ASSERT_NE(frame, nullptr);
    ASSERT_EQ(buffer_pool->unpin_page(frame), RC::SUCCESS);
    LOG_INFO("allocate one page");
  }

  // 5. 分配100个页面并释放50个页面
  const int allocate_page_num2  = 100;
  const int deallocate_page_num = 50;
  for (int i = 1; i <= allocate_page_num2 + deallocate_page_num; i++) {
    if (i % 3 == 0) {
      ASSERT_EQ(buffer_pool->dispose_page(i / 3), RC::SUCCESS);
      LOG_INFO("dispose one page");
    } else {
      Frame *frame = nullptr;
      ASSERT_EQ(RC::SUCCESS, buffer_pool->allocate_page(&frame));
      ASSERT_NE(frame, nullptr);
      ASSERT_EQ(buffer_pool->unpin_page(frame), RC::SUCCESS);
      LOG_INFO("allocate one page");
    }
  }

  // 6. 统计buffer pool的总页面
  ASSERT_EQ(buffer_pool_page_count(buffer_pool), allocate_page_num + allocate_page_num2 - deallocate_page_num);

  // 7. 重启一下检查页面总个数
  ASSERT_EQ(buffer_pool_manager.close_file(buffer_pool_filename.c_str()), RC::SUCCESS);

  ASSERT_EQ(RC::SUCCESS, buffer_pool_manager.open_file(log_handler, buffer_pool_filename.c_str(), buffer_pool));
  ASSERT_NE(buffer_pool, nullptr);
  ASSERT_EQ(buffer_pool_page_count(buffer_pool), allocate_page_num + allocate_page_num2 - deallocate_page_num);

  ASSERT_EQ(buffer_pool_manager.close_file(buffer_pool_filename.c_str()), RC::SUCCESS);
}

TEST(BufferPool, create)
{
  filesystem::path test_directory("buffer_pool");
  filesystem::path bp_file = test_directory / "create.bp";
  filesystem::remove_all(test_directory);
  filesystem::create_directory(test_directory);

  BufferPoolManager bpm;
  ASSERT_EQ(RC::SUCCESS, bpm.init(make_unique<VacuousDoubleWriteBuffer>()));
  ASSERT_EQ(RC::SUCCESS, bpm.create_file(bp_file.c_str()));

  VacuousLogHandler log_handler;
  DiskBufferPool   *buffer_pool = nullptr;
  ASSERT_EQ(RC::SUCCESS, bpm.open_file(log_handler, bp_file.c_str(), buffer_pool));
  ASSERT_NE(buffer_pool, nullptr);

  BufferPoolManager bpm2;
  ASSERT_EQ(RC::SUCCESS, bpm2.init(make_unique<VacuousDoubleWriteBuffer>()));
  filesystem::path bp_file2 = test_directory / "create2.bp";
  filesystem::copy_file(bp_file, bp_file2);

  DiskBufferPool *buffer_pool2 = nullptr;
  ASSERT_EQ(RC::SUCCESS, bpm2.open_file(log_handler, bp_file2.c_str(), buffer_pool2));
  ASSERT_NE(buffer_pool2, nullptr);
  ASSERT_EQ(buffer_pool->id(), buffer_pool2->id());
}

int main(int argc, char **argv)
{
  testing::InitGoogleTest(&argc, argv);
  filesystem::path log_filename = filesystem::path(argv[0]).filename();
  LoggerFactory::init_default(log_filename.string() + ".log", LOG_LEVEL_TRACE);
  return RUN_ALL_TESTS();
}
