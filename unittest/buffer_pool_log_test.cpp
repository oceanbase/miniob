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

#include <algorithm>

#include "gtest/gtest.h"
#include "common/log/log.h"
#include "storage/buffer/disk_buffer_pool.h"
#include "storage/clog/disk_log_handler.h"
#include "storage/buffer/buffer_pool_log.h"
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

TEST(BufferPoolLog, test_wal_normal)
{
  /// 测试正常关闭的情况重做日志

  filesystem::path test_path("test_disk_buffer_pool_wal_normal");
  filesystem::path clog_path = test_path / "clog";

  filesystem::remove_all(test_path);
  filesystem::create_directory(test_path);

  filesystem::path  buffer_pool_filename = test_path / "buffer_pool.bp";
  BufferPoolManager buffer_pool_manager;
  ASSERT_EQ(RC::SUCCESS, buffer_pool_manager.init(make_unique<VacuousDoubleWriteBuffer>()));
  ASSERT_EQ(RC::SUCCESS, buffer_pool_manager.create_file(buffer_pool_filename.c_str()));

  DiskLogHandler  log_handler;
  DiskBufferPool *buffer_pool = nullptr;
  ASSERT_EQ(RC::SUCCESS, buffer_pool_manager.open_file(log_handler, buffer_pool_filename.c_str(), buffer_pool));

  BufferPoolLogReplayer log_replayer(buffer_pool_manager);

  ASSERT_EQ(RC::SUCCESS, log_handler.init(clog_path.c_str()));
  ASSERT_EQ(RC::SUCCESS, log_handler.replay(log_replayer, 0));
  ASSERT_EQ(RC::SUCCESS, log_handler.start());

  const int allocate_page_num = 100;
  for (int i = 0; i < allocate_page_num; i++) {
    Frame *frame = nullptr;
    ASSERT_EQ(RC::SUCCESS, buffer_pool->allocate_page(&frame));
    ASSERT_EQ(RC::SUCCESS, buffer_pool->unpin_page(frame));
  }

  const int allocate_page_num2  = 100;
  const int deallocate_page_num = 50;
  for (int i = 1; i <= allocate_page_num2 + deallocate_page_num; i++) {
    if (i % 3 != 0) {
      Frame *frame = nullptr;
      ASSERT_EQ(RC::SUCCESS, buffer_pool->allocate_page(&frame));
      ASSERT_EQ(RC::SUCCESS, buffer_pool->unpin_page(frame));
    } else {
      ASSERT_EQ(RC::SUCCESS, buffer_pool->dispose_page(i / 3));
    }
  }
  ASSERT_EQ(RC::SUCCESS, log_handler.stop());
  ASSERT_EQ(RC::SUCCESS, log_handler.await_termination());

  ASSERT_EQ(RC::SUCCESS, buffer_pool_manager.close_file(buffer_pool_filename.c_str()));
  buffer_pool = nullptr;

  /// 由于上次是正常关闭，那重新打开后，及时不重做日志，也不会有问题
  /// 这里相当于测试正常关闭的状态下，再重做日志，是否会有问题
  ASSERT_EQ(RC::SUCCESS, buffer_pool_manager.open_file(log_handler, buffer_pool_filename.c_str(), buffer_pool));
  ASSERT_NE(buffer_pool, nullptr);

  DiskLogHandler log_handler2;
  ASSERT_EQ(RC::SUCCESS, log_handler2.init(clog_path.c_str()));
  ASSERT_EQ(RC::SUCCESS, log_handler2.replay(log_replayer, 0));
  ASSERT_EQ(allocate_page_num + allocate_page_num2 - deallocate_page_num, buffer_pool_page_count(buffer_pool));
  ASSERT_EQ(RC::SUCCESS, log_handler2.start());
  ASSERT_EQ(RC::SUCCESS, log_handler2.stop());
  ASSERT_EQ(RC::SUCCESS, log_handler2.await_termination());
}

TEST(BufferPoolLog, test_wal_exception)
{
  /// 测试没有落盘但是日志落盘的情况重做日志
  /*
   * 1. 创建一个buffer pool
   * 2. 分配100个页面
   * 3. 分配100个页面，释放50个页面
   * 4. 关闭buffer pool并删除文件
   * 5. 创建一个新文件，模拟日志没有落地的情况
   * 6. 重做日志
   * 7. 重做后的页面数应该是150
   */
  filesystem::path test_path("test_disk_buffer_pool_wal_exception");
  filesystem::path clog_path = test_path / "clog";

  filesystem::remove_all(test_path);
  filesystem::create_directory(test_path);

  filesystem::path  buffer_pool_filename = test_path / "buffer_pool.bp";
  BufferPoolManager buffer_pool_manager;
  ASSERT_EQ(RC::SUCCESS, buffer_pool_manager.init(make_unique<VacuousDoubleWriteBuffer>()));
  ASSERT_EQ(RC::SUCCESS, buffer_pool_manager.create_file(buffer_pool_filename.c_str()));

  BufferPoolLogReplayer log_replayer(buffer_pool_manager);

  DiskLogHandler  log_handler;
  DiskBufferPool *buffer_pool = nullptr;
  ASSERT_EQ(RC::SUCCESS, buffer_pool_manager.open_file(log_handler, buffer_pool_filename.c_str(), buffer_pool));

  ASSERT_EQ(RC::SUCCESS, log_handler.init(clog_path.c_str()));
  ASSERT_EQ(RC::SUCCESS, log_handler.replay(log_replayer, 0));
  ASSERT_EQ(RC::SUCCESS, log_handler.start());

  const int allocate_page_num = 100;
  for (int i = 0; i < allocate_page_num; i++) {
    Frame *frame = nullptr;
    ASSERT_EQ(RC::SUCCESS, buffer_pool->allocate_page(&frame));
    ASSERT_EQ(RC::SUCCESS, buffer_pool->unpin_page(frame));
  }

  const int allocate_page_num2  = 100;
  const int deallocate_page_num = 50;
  for (int i = 1; i <= allocate_page_num2 + deallocate_page_num; i++) {
    if (i % 3 != 0) {
      Frame *frame = nullptr;
      ASSERT_EQ(RC::SUCCESS, buffer_pool->allocate_page(&frame));
      ASSERT_EQ(RC::SUCCESS, buffer_pool->unpin_page(frame));
    } else {
      ASSERT_EQ(RC::SUCCESS, buffer_pool->dispose_page(i / 3));
    }
  }
  ASSERT_EQ(RC::SUCCESS, log_handler.stop());
  ASSERT_EQ(RC::SUCCESS, log_handler.await_termination());

  ASSERT_EQ(RC::SUCCESS, buffer_pool_manager.close_file(buffer_pool_filename.c_str()));
  buffer_pool = nullptr;

  ASSERT_TRUE(filesystem::remove(buffer_pool_filename));

  BufferPoolManager buffer_pool_manager2;
  ASSERT_EQ(RC::SUCCESS, buffer_pool_manager2.init(make_unique<VacuousDoubleWriteBuffer>()));
  ASSERT_EQ(RC::SUCCESS, buffer_pool_manager2.create_file(buffer_pool_filename.c_str()));
  ASSERT_EQ(RC::SUCCESS, buffer_pool_manager2.open_file(log_handler, buffer_pool_filename.c_str(), buffer_pool));
  ASSERT_NE(buffer_pool, nullptr);

  BufferPoolLogReplayer log_replayer2(buffer_pool_manager2);

  DiskLogHandler log_handler2;
  ASSERT_EQ(RC::SUCCESS, log_handler2.init(clog_path.c_str()));
  LOG_INFO("replay log");
  ASSERT_EQ(RC::SUCCESS, log_handler2.replay(log_replayer2, 0));
  ASSERT_EQ(allocate_page_num + allocate_page_num2 - deallocate_page_num, buffer_pool_page_count(buffer_pool));
  ASSERT_EQ(RC::SUCCESS, log_handler2.start());
  ASSERT_EQ(RC::SUCCESS, log_handler2.stop());
  ASSERT_EQ(RC::SUCCESS, log_handler2.await_termination());
}

TEST(BufferPoolLog, test_wal_exception2)
{
  /// 测试没有落盘但是日志落盘的情况重做日志
  /*
   * 1. 创建一个buffer pool
   * 2. 分配100个页面
   * 3. 正常关闭文件
   * 4. 复制文件保存起来
   * 5. 重新打开日志和这个文件
   * 6. 分配100个页面，释放50个页面
   * 7. 关闭buffer pool并删除文件
   * 8. 复制之前保存的文件
   * 9. 打开复制的文件
   * 10. 重做后的页面数应该是150
   */
  filesystem::path test_path("test_disk_buffer_pool_wal_exception");
  filesystem::path clog_path = test_path / "clog";

  filesystem::remove_all(test_path);
  filesystem::create_directory(test_path);

  filesystem::path  buffer_pool_filename = test_path / "buffer_pool.bp";
  BufferPoolManager buffer_pool_manager;
  ASSERT_EQ(RC::SUCCESS, buffer_pool_manager.init(make_unique<VacuousDoubleWriteBuffer>()));
  ASSERT_EQ(RC::SUCCESS, buffer_pool_manager.create_file(buffer_pool_filename.c_str()));

  BufferPoolLogReplayer log_replayer(buffer_pool_manager);
  DiskBufferPool       *buffer_pool = nullptr;
  DiskLogHandler        log_handler;
  ASSERT_EQ(RC::SUCCESS, buffer_pool_manager.open_file(log_handler, buffer_pool_filename.c_str(), buffer_pool));

  ASSERT_EQ(RC::SUCCESS, log_handler.init(clog_path.c_str()));
  ASSERT_EQ(RC::SUCCESS, log_handler.replay(log_replayer, 0));
  ASSERT_EQ(RC::SUCCESS, log_handler.start());

  /// step 2,3 创建100个页面然后正常关闭，包括关闭日志
  const int allocate_page_num = 100;
  for (int i = 0; i < allocate_page_num; i++) {
    Frame *frame = nullptr;
    ASSERT_EQ(RC::SUCCESS, buffer_pool->allocate_page(&frame));
    ASSERT_EQ(RC::SUCCESS, buffer_pool->unpin_page(frame));
  }

  ASSERT_EQ(RC::SUCCESS, log_handler.stop());
  ASSERT_EQ(RC::SUCCESS, log_handler.await_termination());

  ASSERT_EQ(RC::SUCCESS, buffer_pool_manager.close_file(buffer_pool_filename.c_str()));
  buffer_pool = nullptr;

  /// step 4 备份文件
  filesystem::path buffer_pool_filename_backup = test_path / "buffer_pool_backup.bp";
  ASSERT_TRUE(filesystem::copy_file(buffer_pool_filename, buffer_pool_filename_backup));

  /// step 5 重新打开日志和文件
  DiskLogHandler log_handler2;

  ASSERT_EQ(RC::SUCCESS, buffer_pool_manager.open_file(log_handler2, buffer_pool_filename.c_str(), buffer_pool));
  ASSERT_NE(buffer_pool, nullptr);
  ASSERT_EQ(buffer_pool_page_count(buffer_pool), allocate_page_num);
  ASSERT_EQ(RC::SUCCESS, log_handler2.init(clog_path.c_str()));
  ASSERT_EQ(RC::SUCCESS, log_handler2.replay(log_replayer, 0));
  ASSERT_EQ(buffer_pool_page_count(buffer_pool), allocate_page_num);
  ASSERT_EQ(RC::SUCCESS, log_handler2.start());

  /// step 6 分配100个页面，释放50个页面
  const int allocate_page_num2  = 100;
  const int deallocate_page_num = 50;
  for (int i = 1; i <= allocate_page_num2 + deallocate_page_num; i++) {
    if (i % 3 != 0) {
      Frame *frame = nullptr;
      ASSERT_EQ(RC::SUCCESS, buffer_pool->allocate_page(&frame));
      ASSERT_EQ(RC::SUCCESS, buffer_pool->unpin_page(frame));
    } else {
      ASSERT_EQ(RC::SUCCESS, buffer_pool->dispose_page(i / 3));
    }
  }
  ASSERT_EQ(RC::SUCCESS, log_handler2.stop());
  ASSERT_EQ(RC::SUCCESS, log_handler2.await_termination());

  ASSERT_EQ(allocate_page_num + allocate_page_num2 - deallocate_page_num, buffer_pool_page_count(buffer_pool));

  /// step 7 关闭旧文件并删除他
  ASSERT_EQ(RC::SUCCESS, buffer_pool_manager.close_file(buffer_pool_filename.c_str()));
  buffer_pool = nullptr;
  ASSERT_TRUE(filesystem::remove(buffer_pool_filename));

  /// step 8 复制备份的文件
  ASSERT_TRUE(filesystem::copy_file(buffer_pool_filename_backup, buffer_pool_filename));

  /// step 9 打开复制后的文件，当前的文件页面数应该是100
  DiskLogHandler log_handler3;
  ASSERT_EQ(RC::SUCCESS, buffer_pool_manager.open_file(log_handler3, buffer_pool_filename.c_str(), buffer_pool));
  ASSERT_NE(buffer_pool, nullptr);

  /// step 10 重做日志，应该有150个页面
  ASSERT_EQ(RC::SUCCESS, log_handler3.init(clog_path.c_str()));
  LOG_INFO("before the log handler 3 replay");
  ASSERT_EQ(RC::SUCCESS, log_handler3.replay(log_replayer, 0));
  ASSERT_EQ(allocate_page_num + allocate_page_num2 - deallocate_page_num, buffer_pool_page_count(buffer_pool));
  ASSERT_EQ(RC::SUCCESS, log_handler3.start());
  ASSERT_EQ(RC::SUCCESS, buffer_pool_manager.close_file(buffer_pool_filename.c_str()));
  buffer_pool = nullptr;
  ASSERT_EQ(RC::SUCCESS, log_handler3.stop());
  ASSERT_EQ(RC::SUCCESS, log_handler3.await_termination());
}

TEST(BufferPoolLog, test_wal_multi_files)
{
  /// 测试多个buffer pool文件，没有正常落盘但是日志落地的情况。使用重建文件的方式模拟异常情况
  /*
   * 1. 创建10个buffer pool文件
   * 2. 每个文件分配100个页面
   * 3. 每个分配100个页面，释放50个页面
   * 4. 关闭所有buffer pool文件并删除
   * 5. 创建10个新文件，模拟日志没有落地的情况
   * 6. 重做日志
   * 7. 重做后每个文件页面数应该是150
   */
  filesystem::path test_path("test_disk_buffer_pool_wal_exception");
  filesystem::path clog_path = test_path / "clog";

  filesystem::remove_all(test_path);
  filesystem::create_directory(test_path);

  vector<filesystem::path> buffer_pool_filenames;
  const int                buffer_pool_file_num = 10;
  for (int i = 0; i < buffer_pool_file_num; i++) {
    filesystem::path buffer_pool_filename = test_path / ("buffer_pool_" + to_string(i) + ".bp");
    buffer_pool_filenames.push_back(buffer_pool_filename);
  }

  BufferPoolManager buffer_pool_manager;
  ASSERT_EQ(RC::SUCCESS, buffer_pool_manager.init(make_unique<VacuousDoubleWriteBuffer>()));
  ranges::for_each(buffer_pool_filenames, [&buffer_pool_manager](const filesystem::path &filename) {
    ASSERT_EQ(RC::SUCCESS, buffer_pool_manager.create_file(filename.c_str()));
  });

  DiskLogHandler log_handler;

  vector<DiskBufferPool *> buffer_pools;
  ranges::for_each(
      buffer_pool_filenames, [&buffer_pool_manager, &buffer_pools, &log_handler](const filesystem::path &filename) {
        DiskBufferPool *buffer_pool = nullptr;
        ASSERT_EQ(RC::SUCCESS, buffer_pool_manager.open_file(log_handler, filename.c_str(), buffer_pool));
        buffer_pools.push_back(buffer_pool);
      });

  BufferPoolLogReplayer log_replayer(buffer_pool_manager);

  ASSERT_EQ(RC::SUCCESS, log_handler.init(clog_path.c_str()));
  ASSERT_EQ(RC::SUCCESS, log_handler.replay(log_replayer, 0));
  ASSERT_EQ(RC::SUCCESS, log_handler.start());

  const int allocate_page_num = 100;
  for (int i = 0; i < allocate_page_num; i++) {
    ranges::for_each(buffer_pools, [](DiskBufferPool *buffer_pool) {
      Frame *frame = nullptr;
      ASSERT_EQ(RC::SUCCESS, buffer_pool->allocate_page(&frame));
      ASSERT_EQ(RC::SUCCESS, buffer_pool->unpin_page(frame));
    });
  }

  const int allocate_page_num2  = 100;
  const int deallocate_page_num = 50;
  for (int i = 1; i <= allocate_page_num2 + deallocate_page_num; i++) {
    if (i % 3 != 0) {
      ranges::for_each(buffer_pools, [](DiskBufferPool *buffer_pool) {
        Frame *frame = nullptr;
        ASSERT_EQ(RC::SUCCESS, buffer_pool->allocate_page(&frame));
        ASSERT_EQ(RC::SUCCESS, buffer_pool->unpin_page(frame));
      });
    } else {
      ranges::for_each(
          buffer_pools, [i](DiskBufferPool *buffer_pool) { ASSERT_EQ(RC::SUCCESS, buffer_pool->dispose_page(i / 3)); });
    }
  }
  ASSERT_EQ(RC::SUCCESS, log_handler.stop());
  ASSERT_EQ(RC::SUCCESS, log_handler.await_termination());

  ranges::for_each(buffer_pool_filenames, [&buffer_pool_manager](const filesystem::path &filename) {
    ASSERT_EQ(RC::SUCCESS, buffer_pool_manager.close_file(filename.c_str()));
  });
  buffer_pools.clear();

  ranges::for_each(
      buffer_pool_filenames, [](const filesystem::path &filename) { ASSERT_TRUE(filesystem::remove(filename)); });

  BufferPoolManager buffer_pool_manager2;
  ASSERT_EQ(RC::SUCCESS, buffer_pool_manager2.init(make_unique<VacuousDoubleWriteBuffer>()));
  ranges::for_each(buffer_pool_filenames, [&buffer_pool_manager2](const filesystem::path &filename) {
    ASSERT_EQ(RC::SUCCESS, buffer_pool_manager2.create_file(filename.c_str()));
  });

  DiskLogHandler log_handler2;
  ranges::for_each(
      buffer_pool_filenames, [&buffer_pool_manager2, &buffer_pools, &log_handler2](const filesystem::path &filename) {
        DiskBufferPool *buffer_pool = nullptr;
        ASSERT_EQ(RC::SUCCESS, buffer_pool_manager2.open_file(log_handler2, filename.c_str(), buffer_pool));
        ASSERT_NE(buffer_pool, nullptr);
        buffer_pools.push_back(buffer_pool);
      });

  BufferPoolLogReplayer log_replayer2(buffer_pool_manager2);
  ASSERT_EQ(RC::SUCCESS, log_handler2.init(clog_path.c_str()));
  ASSERT_EQ(RC::SUCCESS, log_handler2.replay(log_replayer2, 0));
  ranges::for_each(buffer_pools, [](DiskBufferPool *buffer_pool) {
    ASSERT_EQ(allocate_page_num + allocate_page_num2 - deallocate_page_num, buffer_pool_page_count(buffer_pool));
  });
  ASSERT_EQ(RC::SUCCESS, log_handler2.start());
  ranges::for_each(buffer_pool_filenames, [&buffer_pool_manager2](const filesystem::path &filename) {
    ASSERT_EQ(RC::SUCCESS, buffer_pool_manager2.close_file(filename.c_str()));
  });
  buffer_pools.clear();
  ASSERT_EQ(RC::SUCCESS, log_handler2.stop());
  ASSERT_EQ(RC::SUCCESS, log_handler2.await_termination());
}

int main(int argc, char **argv)
{
  testing::InitGoogleTest(&argc, argv);
  filesystem::path log_filename = filesystem::path(argv[0]).filename();
  LoggerFactory::init_default(log_filename.string() + ".log", LOG_LEVEL_TRACE);
  return RUN_ALL_TESTS();
}
