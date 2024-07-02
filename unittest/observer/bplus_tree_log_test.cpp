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
// Created by wangyunlai.wyl on 2024/02/21
//

#include <filesystem>
#include <algorithm>
#include <random>

#include "gtest/gtest.h"
#include "storage/index/bplus_tree_log.h"
#include "storage/index/bplus_tree.h"
#include "storage/clog/disk_log_handler.h"
#include "storage/clog/integrated_log_replayer.h"
#include "common/math/integer_generator.h"
#include "common/thread/thread_pool_executor.h"
#include "storage/buffer/double_write_buffer.h"

using namespace std;
using namespace common;

RC list_all_values(BplusTreeHandler &tree_handler, vector<RID> &rids)
{
  auto scanner = make_unique<BplusTreeScanner>(tree_handler);
  RC   rc      = scanner->open(nullptr /*left_user_key*/,
      0 /*left_len*/,
      true /*left_inclusive*/,
      nullptr /*right_user_key*/,
      0 /*right_len*/,
      true /*right_inclusive*/);
  if (OB_SUCC(rc)) {
    RID rid;
    while (OB_SUCC(scanner->next_entry(rid))) {
      rids.push_back(rid);
    }
    rc = scanner->close();
  }
  return rc;
}

TEST(BplusTreeLog, base)
{
  filesystem::path test_directory = "bplus_tree_log_test_dir";
  filesystem::remove_all(test_directory);
  filesystem::create_directory(test_directory);

  const filesystem::path bp_filename = test_directory / "bplus_tree.bp";

  // NOTE: 这里的代码使用unique_ptr，是为了方便控制它们释放的顺序

  // 1. create a bplus tree and a disk logger
  auto bpm = make_unique<BufferPoolManager>();
  ASSERT_EQ(RC::SUCCESS, bpm->init(make_unique<VacuousDoubleWriteBuffer>()));
  DiskBufferPool *buffer_pool = nullptr;
  auto            log_handler = make_unique<DiskLogHandler>();
  ASSERT_EQ(RC::SUCCESS, bpm->create_file(bp_filename.c_str()));
  ASSERT_EQ(RC::SUCCESS, bpm->open_file(*log_handler, bp_filename.c_str(), buffer_pool));
  ASSERT_NE(nullptr, buffer_pool);

  filesystem::path log_directory = test_directory / "clog";
  ASSERT_EQ(RC::SUCCESS, log_handler->init(log_directory.c_str()));

  IntegratedLogReplayer log_replayer(*bpm);
  ASSERT_EQ(RC::SUCCESS, log_handler->replay(log_replayer, 0));
  ASSERT_EQ(RC::SUCCESS, log_handler->start());

  auto bplus_tree = make_unique<BplusTreeHandler>();
  ASSERT_EQ(RC::SUCCESS, bplus_tree->create(*log_handler, *buffer_pool, AttrType::INTS, 4));

  // 2. insert some key-value pairs into the bplus tree
  const int   insert_num = 10000;
  vector<int> keys(insert_num);
  for (int i = 0; i < insert_num; i++) {
    keys[i] = i;
  }

  random_device rd;
  mt19937       generator(rd());
  shuffle(keys.begin(), keys.end(), generator);

  for (int i : keys) {
    RID rid(i, i);
    int key = i;
    ASSERT_EQ(RC::SUCCESS, bplus_tree->insert_entry(reinterpret_cast<const char *>(&key), &rid));
  }

  // 3. write logs to disk
  ASSERT_EQ(log_handler->stop(), RC::SUCCESS);
  ASSERT_EQ(log_handler->await_termination(), RC::SUCCESS);

  bplus_tree.reset();
  bpm.reset();
  log_handler.reset();

  // 4. close the bplus tree and the disk logger
  // copy the old buffer pool file
  const filesystem::path bp_filename2 = test_directory / "bplus_tree2.bp";
  ASSERT_TRUE(filesystem::copy_file(bp_filename, bp_filename2));

  LOG_INFO("begin to recover the files");

  auto bpm2 = make_unique<BufferPoolManager>();
  ASSERT_EQ(RC::SUCCESS, bpm2->init(make_unique<VacuousDoubleWriteBuffer>()));
  auto            log_handler2 = make_unique<DiskLogHandler>();
  DiskBufferPool *buffer_pool2 = nullptr;
  ASSERT_EQ(RC::SUCCESS, bpm2->open_file(*log_handler2, bp_filename2.c_str(), buffer_pool2));
  ASSERT_NE(nullptr, buffer_pool2);

  // 5. open the bplus tree and the disk logger
  ASSERT_EQ(RC::SUCCESS, log_handler2->init(log_directory.c_str()));

  // 6. replay logs from disk
  IntegratedLogReplayer log_replayer2(*bpm2);
  ASSERT_EQ(RC::SUCCESS, log_handler2->replay(log_replayer2, 0));

  // 7. check the bplus tree
  auto tree_handler2 = make_unique<BplusTreeHandler>();
  ASSERT_EQ(RC::SUCCESS, tree_handler2->open(*log_handler2, *buffer_pool2));

  auto scanner = make_unique<BplusTreeScanner>(*tree_handler2);
  ASSERT_EQ(RC::SUCCESS,
      scanner->open(nullptr /*left_user_key*/,
          0 /*left_len*/,
          true /*left_inclusive*/,
          nullptr /*right_user_key*/,
          0 /*right_len*/,
          true /*right_inclusive*/));

  RC          rc = RC::SUCCESS;
  vector<RID> rids;
  RID         rid;
  while (OB_SUCC(rc = scanner->next_entry(rid))) {
    rids.push_back(rid);
  }
  ASSERT_EQ(RC::RECORD_EOF, rc);
  ASSERT_EQ(RC::SUCCESS, scanner->close());

  ASSERT_EQ(insert_num, rids.size());
  for (int i = 0; i < insert_num; i++) {
    ASSERT_EQ(i, rids[i].page_num);
    ASSERT_EQ(i, rids[i].slot_num);
  }

  scanner.reset();
  tree_handler2.reset();
  bpm2.reset();
  log_handler2.reset();
}

TEST(BplusTreeLog, concurrency)
{
  filesystem::path test_directory      = "bplus_tree_log_test_dir";
  filesystem::path child_directory_src = test_directory / "src";
  filesystem::path child_directory_dst = test_directory / "dst";
  filesystem::remove_all(test_directory);
  filesystem::create_directories(child_directory_src);

  vector<filesystem::path> bp_filenames;
  for (int i = 0; i < 10; i++) {
    bp_filenames.push_back(child_directory_src / ("bplus_tree" + to_string(i) + ".bp"));
  }

  // 创建一批B+树，执行插入、删除动作
  vector<DiskBufferPool *> buffer_pools;
  auto                     bpm = make_unique<BufferPoolManager>();
  ASSERT_EQ(RC::SUCCESS, bpm->init(make_unique<VacuousDoubleWriteBuffer>()));
  auto log_handler = make_unique<DiskLogHandler>();
  for (filesystem::path &bp_filename : bp_filenames) {
    ASSERT_EQ(RC::SUCCESS, bpm->create_file(bp_filename.c_str()));
    DiskBufferPool *buffer_pool = nullptr;
    ASSERT_EQ(RC::SUCCESS, bpm->open_file(*log_handler, bp_filename.c_str(), buffer_pool));
    ASSERT_NE(nullptr, buffer_pool);
    buffer_pools.push_back(buffer_pool);
  }

  filesystem::path log_directory = child_directory_src / "clog";
  ASSERT_EQ(RC::SUCCESS, log_handler->init(log_directory.c_str()));

  IntegratedLogReplayer log_replayer(*bpm);
  ASSERT_EQ(RC::SUCCESS, log_handler->replay(log_replayer, 0));
  ASSERT_EQ(RC::SUCCESS, log_handler->start());

  vector<unique_ptr<BplusTreeHandler>> bplus_trees;
  for (DiskBufferPool *buffer_pool : buffer_pools) {
    auto bplus_tree = make_unique<BplusTreeHandler>();
    ASSERT_EQ(RC::SUCCESS, bplus_tree->create(*log_handler, *buffer_pool, AttrType::INTS, 4));
    bplus_trees.push_back(std::move(bplus_tree));
  }

  const int   insert_num = 1000 * static_cast<int>(bp_filenames.size());
  vector<int> keys(insert_num);
  for (int i = 0; i < insert_num; i++) {
    keys[i] = i;
  }

  random_device rd;
  mt19937       generator(rd());
  shuffle(keys.begin(), keys.end(), generator);

  IntegerGenerator tree_index_generator(0, static_cast<int>(bplus_trees.size() - 1) /*max_value*/);

  ThreadPoolExecutor executor;
  ASSERT_EQ(0, executor.init("test", 4, 8, 60 * 1000));

  for (int i : keys) {

    executor.execute([&bplus_trees, &tree_index_generator, i]() {
      RID rid(i, i);
      int tree_index = tree_index_generator.next();
      ASSERT_EQ(RC::SUCCESS, bplus_trees[tree_index]->insert_entry(reinterpret_cast<const char *>(&i), &rid));
    });
  }

  const int        random_operation_num = 1000 * static_cast<int>(bp_filenames.size());
  IntegerGenerator operation_index_generator(0, 1);  // 0 for insertion, 1 for deletion
  for (int i = 0; i < random_operation_num; i++) {
    executor.execute([&bplus_trees, &tree_index_generator, &operation_index_generator, i]() {
      int tree_index      = tree_index_generator.next();
      int operation_index = operation_index_generator.next();
      RID rid(i, i);
      if (0 == operation_index) {
        bplus_trees[tree_index]->insert_entry(reinterpret_cast<const char *>(&i), &rid);
      } else {
        bplus_trees[tree_index]->delete_entry(reinterpret_cast<const char *>(&i), &rid);
      }
    });
  }

  ASSERT_EQ(0, executor.shutdown());
  ASSERT_EQ(0, executor.await_termination());

  // write logs to disk
  ASSERT_EQ(log_handler->stop(), RC::SUCCESS);
  ASSERT_EQ(log_handler->await_termination(), RC::SUCCESS);

  // copy all files from src to dst
  error_code ec;
  filesystem::copy(child_directory_src, child_directory_dst, filesystem::copy_options::recursive, ec);
  ASSERT_EQ(ec.value(), 0);

  // open another context
  // 创建另一个环境，把上一份文件内容，包括buffer pool文件和日志文件
  // 都复制过来，然后尝试恢复，再对比两边数据是否一致
  LOG_INFO("copy the old files into new directory and try to recover them");
  auto bpm2 = make_unique<BufferPoolManager>();
  ASSERT_EQ(RC::SUCCESS, bpm2->init(make_unique<VacuousDoubleWriteBuffer>()));
  auto                     log_handler2 = make_unique<DiskLogHandler>();
  vector<DiskBufferPool *> buffer_pools2;
  vector<filesystem::path> bp_filenames2;
  for (int i = 0; i < 10; i++) {
    bp_filenames2.push_back(child_directory_dst / ("bplus_tree" + to_string(i) + ".bp"));
  }
  for (filesystem::path &bp_filename : bp_filenames2) {
    DiskBufferPool *buffer_pool = nullptr;
    ASSERT_EQ(RC::SUCCESS, bpm2->open_file(*log_handler2, bp_filename.c_str(), buffer_pool));
    ASSERT_NE(nullptr, buffer_pool);
    buffer_pools2.push_back(buffer_pool);
  }

  ASSERT_EQ(RC::SUCCESS, log_handler2->init(log_directory.c_str()));
  IntegratedLogReplayer log_replayer2(*bpm2);
  ASSERT_EQ(RC::SUCCESS, log_handler2->replay(log_replayer2, 0));

  vector<unique_ptr<BplusTreeHandler>> bplus_trees2;
  for (DiskBufferPool *buffer_pool : buffer_pools2) {
    auto bplus_tree = make_unique<BplusTreeHandler>();
    ASSERT_EQ(RC::SUCCESS, bplus_tree->open(*log_handler2, *buffer_pool));
    bplus_trees2.push_back(std::move(bplus_tree));
  }

  // compare the bplus trees
  for (int i = 0; i < static_cast<int>(bplus_trees.size()); i++) {
    vector<RID> rids1, rids2;
    ASSERT_EQ(RC::SUCCESS, list_all_values(*bplus_trees[i], rids1));
    ASSERT_EQ(RC::SUCCESS, list_all_values(*bplus_trees2[i], rids2));
    LOG_INFO("bplus tree log test. tree index: %d, rids1 size: %d, rids2 size: %d", i, rids1.size(), rids2.size());
    if (rids1.size() != rids2.size()) {
      for (RID &rid : rids1) {
        LOG_INFO("hnwyllmm rid1: %s", rid.to_string().c_str());
      }
      for (RID &rid : rids2) {
        LOG_INFO("hnwyllmm rid2: %s", rid.to_string().c_str());
      }
    }
    ASSERT_EQ(rids1.size(), rids2.size());
    for (int j = 0; j < static_cast<int>(rids1.size()); j++) {
      ASSERT_EQ(rids1[j], rids2[j]);
    }
  }

  // clear all resources
  bplus_trees2.clear();
  LOG_INFO("bplus_tree2 destoried");

  bpm2.reset();
  LOG_INFO("bpm2 destoried");

  log_handler2.reset();
  LOG_INFO("log_handler2 destoried");

  bplus_trees.clear();
  LOG_INFO("bplus_trees destoried");

  bpm.reset();
  LOG_INFO("bpm destoried");

  log_handler.reset();
  LOG_INFO("log_handler destoried");
}

int main(int argc, char **argv)
{
  testing::InitGoogleTest(&argc, argv);
  filesystem::path log_filename = filesystem::path(argv[0]).filename();
  LoggerFactory::init_default(log_filename.string() + ".log", LOG_LEVEL_TRACE);
  return RUN_ALL_TESTS();
}
