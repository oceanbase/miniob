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
// Created by wangyunlai on 2024/04/19
//

#include <filesystem>

#include "gtest/gtest.h"

#include "storage/buffer/disk_buffer_pool.h"
#include "storage/buffer/double_write_buffer.h"
#include "storage/clog/vacuous_log_handler.h"
#include "storage/clog/disk_log_handler.h"
#include "storage/clog/integrated_log_replayer.h"

using namespace std;
using namespace common;

TEST(DoubleWriteBuffer, single_file_normal)
{
  /*
  测试单个文件，
  分配一些页面
  释放一些页面
  正常停止
  正常启动
  然后检测重启后是否与之前分配的页面一致
  */
  filesystem::path directory("double_write_buffer_test_dir");
  filesystem::remove_all(directory);
  filesystem::create_directories(directory);

  filesystem::path buffer_pool_filename         = directory / "buffer_pool.bp";
  filesystem::path double_write_buffer_filename = directory / "double_write_buffer.dwb";

  auto              bpm = make_unique<BufferPoolManager>();
  VacuousLogHandler log_handler;
  auto              double_write_buffer = make_unique<DiskDoubleWriteBuffer>(*bpm);
  ASSERT_EQ(RC::SUCCESS, double_write_buffer->open_file(double_write_buffer_filename.c_str()));
  ASSERT_EQ(bpm->init(std::move(double_write_buffer)), RC::SUCCESS);

  DiskBufferPool *buffer_pool = nullptr;
  ASSERT_EQ(RC::SUCCESS, bpm->create_file(buffer_pool_filename.c_str()));
  ASSERT_EQ(RC::SUCCESS, bpm->open_file(log_handler, buffer_pool_filename.c_str(), buffer_pool));
  ASSERT_NE(buffer_pool, nullptr);

  vector<FrameId> allocated_frame_ids;
  vector<FrameId> dispose_frame_ids;
  int             allocate_frame_num = 100;
  for (int i = 0; i < allocate_frame_num; i++) {
    Frame *frame = nullptr;
    ASSERT_EQ(RC::SUCCESS, buffer_pool->allocate_page(&frame));
    ASSERT_NE(frame, nullptr);
    if (i % 2 == 1) {
      allocated_frame_ids.push_back(frame->frame_id());
    } else {
      dispose_frame_ids.push_back(frame->frame_id());
    }

    frame->unpin();
  }

  for (FrameId &frame_id : dispose_frame_ids) {
    PageNum page_num = frame_id.page_num();
    ASSERT_EQ(RC::SUCCESS, buffer_pool->dispose_page(page_num));
  }

  bpm = nullptr;
  LOG_INFO("close buffer pool manager");

  LOG_INFO("reopen the buffer pool and check it");
  bpm                 = make_unique<BufferPoolManager>();
  double_write_buffer = make_unique<DiskDoubleWriteBuffer>(*bpm);
  ASSERT_EQ(RC::SUCCESS, double_write_buffer->open_file(double_write_buffer_filename.c_str()));
  ASSERT_EQ(bpm->init(std::move(double_write_buffer)), RC::SUCCESS);

  ASSERT_EQ(RC::SUCCESS, bpm->open_file(log_handler, buffer_pool_filename.c_str(), buffer_pool));
  ASSERT_NE(buffer_pool, nullptr);

  DiskDoubleWriteBuffer *disk_double_write_buffer = static_cast<DiskDoubleWriteBuffer *>(bpm->get_dblwr_buffer());
  ASSERT_EQ(RC::SUCCESS, disk_double_write_buffer->recover());

  BufferPoolIterator bp_iterator;
  ASSERT_EQ(RC::SUCCESS, bp_iterator.init(*buffer_pool, 1));
  vector<FrameId> collected_frames;
  while (bp_iterator.has_next()) {
    PageNum page_num = bp_iterator.next();
    collected_frames.push_back(FrameId(buffer_pool->id(), page_num));
  }
  bpm = nullptr;

  ASSERT_EQ(allocated_frame_ids, collected_frames);
}

TEST(DoubleWriteBuffer, single_file_exception)
{
  /*
  测试单个文件，
  分配一些页面
  释放一些页面
  模拟异常停止
  正常启动
  然后检测重启后是否与之前分配的页面一致
  */
  filesystem::path directory("double_write_buffer_test_single_file_exception_dir");
  filesystem::remove_all(directory);
  filesystem::create_directories(directory);

  filesystem::path src_path = directory / "src";
  filesystem::create_directories(src_path);
  filesystem::path dst_path = directory / "dst";
  filesystem::create_directories(dst_path);
  filesystem::path buffer_pool_filename         = src_path / "buffer_pool.bp";
  filesystem::path double_write_buffer_filename = src_path / "double_write_buffer.dwb";

  filesystem::path buffer_pool_filename2         = dst_path / "buffer_pool.bp";
  filesystem::path double_write_buffer_filename2 = dst_path / "double_write_buffer.dwb";

  auto           bpm = make_unique<BufferPoolManager>();
  DiskLogHandler log_handler;
  ASSERT_EQ(RC::SUCCESS, log_handler.init((src_path / "clog").c_str()));

  auto double_write_buffer = make_unique<DiskDoubleWriteBuffer>(*bpm);
  ASSERT_EQ(RC::SUCCESS, double_write_buffer->open_file(double_write_buffer_filename.c_str()));
  ASSERT_EQ(bpm->init(std::move(double_write_buffer)), RC::SUCCESS);

  ASSERT_EQ(RC::SUCCESS, log_handler.start());

  DiskBufferPool *buffer_pool = nullptr;
  ASSERT_EQ(RC::SUCCESS, bpm->create_file(buffer_pool_filename.c_str()));
  ASSERT_EQ(RC::SUCCESS, bpm->open_file(log_handler, buffer_pool_filename.c_str(), buffer_pool));
  ASSERT_NE(buffer_pool, nullptr);

  vector<FrameId> allocated_frame_ids;
  vector<FrameId> dispose_frame_ids;
  int             allocate_frame_num = 100;
  for (int i = 0; i < allocate_frame_num; i++) {
    Frame *frame = nullptr;
    ASSERT_EQ(RC::SUCCESS, buffer_pool->allocate_page(&frame));
    ASSERT_NE(frame, nullptr);
    if (i % 2 == 1) {
      allocated_frame_ids.push_back(frame->frame_id());
    } else {
      dispose_frame_ids.push_back(frame->frame_id());
    }

    frame->unpin();
  }

  for (FrameId &frame_id : dispose_frame_ids) {
    PageNum page_num = frame_id.page_num();
    ASSERT_EQ(RC::SUCCESS, buffer_pool->dispose_page(page_num));
  }

  // bpm = nullptr;
  log_handler.stop();
  log_handler.await_termination();
  LOG_INFO("cannot close bpm");

  LOG_INFO("reopen the buffer pool and check it");
  filesystem::copy(src_path, dst_path, filesystem::copy_options::recursive);

  DiskLogHandler log_handler2;
  ASSERT_EQ(log_handler2.init((dst_path / "clog").c_str()), RC::SUCCESS);

  auto bpm2                 = make_unique<BufferPoolManager>();
  auto double_write_buffer2 = make_unique<DiskDoubleWriteBuffer>(*bpm2);
  ASSERT_EQ(RC::SUCCESS, double_write_buffer2->open_file(double_write_buffer_filename2.c_str()));
  ASSERT_EQ(bpm2->init(std::move(double_write_buffer2)), RC::SUCCESS);

  DiskBufferPool *buffer_pool2 = nullptr;
  ASSERT_EQ(RC::SUCCESS, bpm2->open_file(log_handler2, buffer_pool_filename2.c_str(), buffer_pool2));
  ASSERT_NE(buffer_pool2, nullptr);

  DiskDoubleWriteBuffer *disk_double_write_buffer = static_cast<DiskDoubleWriteBuffer *>(bpm2->get_dblwr_buffer());
  ASSERT_EQ(RC::SUCCESS, disk_double_write_buffer->recover());

  IntegratedLogReplayer log_replayer(*bpm2);
  ASSERT_EQ(RC::SUCCESS, log_handler2.replay(log_replayer, 0));

  BufferPoolIterator bp_iterator;
  ASSERT_EQ(RC::SUCCESS, bp_iterator.init(*buffer_pool2, 1));
  vector<FrameId> collected_frames;
  while (bp_iterator.has_next()) {
    PageNum page_num = bp_iterator.next();
    collected_frames.push_back(FrameId(buffer_pool2->id(), page_num));
  }

  ASSERT_EQ(allocated_frame_ids, collected_frames);

  bpm2 = nullptr;
  bpm  = nullptr;
}

int main(int argc, char **argv)
{
  testing::InitGoogleTest(&argc, argv);
  LoggerFactory::init_default(string(argv[0]) + ".log", LOG_LEVEL_TRACE);
  return RUN_ALL_TESTS();
}
