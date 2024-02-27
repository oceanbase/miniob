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

using namespace std;
using namespace common;

TEST(BplusTreeLog, base)
{
  filesystem::path test_directory = "bplus_tree_log_test";
  filesystem::remove_all(test_directory);
  filesystem::create_directory(test_directory);

  const filesystem::path bp_filename = test_directory / "bplus_tree.bp";

  // 1. create a bplus tree and a disk logger
  auto bpm = make_unique<BufferPoolManager>();
  DiskBufferPool *buffer_pool = nullptr;
  auto log_handler = make_unique<DiskLogHandler>();
  ASSERT_EQ(RC::SUCCESS, bpm->create_file(bp_filename.c_str()));
  ASSERT_EQ(RC::SUCCESS, bpm->open_file(*log_handler, bp_filename.c_str(), buffer_pool));
  ASSERT_NE(nullptr, buffer_pool);
  
  filesystem::path log_directory = test_directory / "clog";
  ASSERT_EQ(RC::SUCCESS, log_handler->init(log_directory.c_str()));

  IntegratedLogReplayer log_replayer(*bpm);
  ASSERT_EQ(RC::SUCCESS, log_handler->replay(log_replayer, 0));
  ASSERT_EQ(RC::SUCCESS, log_handler->start());

  auto bplus_tree = make_unique<BplusTreeHandler>();
  ASSERT_EQ(RC::SUCCESS, bplus_tree->create(*log_handler, *buffer_pool, INTS, 4));
  
  // 2. insert some key-value pairs into the bplus tree
  const int insert_num = 10000;
  vector<int> keys(insert_num);
  for (int i = 0; i < insert_num; i++) {
    keys[i] = i;
  }

  random_device rd;
  mt19937 generator(rd());
  shuffle(keys.begin(), keys.end(), generator);

  for (int i : keys) {
    RID rid(i, i);
    int key = i;
    ASSERT_EQ(RC::SUCCESS, bplus_tree->insert_entry(reinterpret_cast<const char *>(&key), &rid));
  }

  // 3. write logs to disk
  ASSERT_EQ(log_handler->stop(), RC::SUCCESS);
  ASSERT_EQ(log_handler->wait(), RC::SUCCESS);

  bplus_tree.reset();
  bpm.reset();
  log_handler.reset();

  // 4. close the bplus tree and the disk logger
  // copy the old buffer pool file
  const filesystem::path bp_filename2 = test_directory / "bplus_tree2.bp";
  ASSERT_TRUE(filesystem::copy_file(bp_filename, bp_filename2));

  auto bpm2 = make_unique<BufferPoolManager>();
  auto log_handler2 = make_unique<DiskLogHandler>();
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
  ASSERT_EQ(RC::SUCCESS, scanner->open(nullptr/*left_user_key*/, 0/*left_len*/, true/*left_inclusive*/, nullptr/*right_user_key*/, 0/*right_len*/, true/*right_inclusive*/));

  RC rc = RC::SUCCESS;
  vector<RID> rids;
  RID rid;
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

int main(int argc, char **argv)
{
  testing::InitGoogleTest(&argc, argv);
  filesystem::path log_filename = filesystem::path(argv[0]).filename();
  LoggerFactory::init_default(log_filename.string() + ".log", LOG_LEVEL_TRACE);
  return RUN_ALL_TESTS();
}
