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
// Created by Ping Xu(haibarapink@gmail.com) on 2025/2/4.
//
#include "gtest/gtest.h"

#include "common/lang/filesystem.h"
#include "oblsm/wal/ob_lsm_wal.h"
#include "oblsm/include/ob_lsm.h"
#include "oblsm/include/ob_lsm_options.h"
#include "oblsm/ob_lsm_define.h"

using namespace oceanbase;

TEST(wal, DISABLED_basic_test)
{
  filesystem::remove_all("oblsm_tmp");
  filesystem::create_directory("oblsm_tmp");
  auto path    = filesystem::path("oblsm_tmp");
  auto rw_file = path / "tmp.wal";
  WAL  wal;
  EXPECT_EQ(wal.open(rw_file), RC::SUCCESS);

  int count = 10000;
  for (auto i = 0; i < count; ++i) {
    auto k = "key" + std::to_string(i);
    auto v = "val" + std::to_string(i);
    EXPECT_EQ(wal.put(i, k, v), RC::SUCCESS);
    wal.sync();
  }

  // Recover from wal
  std::vector<WalRecord> records;
  EXPECT_EQ(wal.recover(rw_file, records), RC::SUCCESS);

  int p = 0;
  for (auto [seq, k, v] : records) {
    auto cur_k = "key" + std::to_string(p);
    auto cur_v = "val" + std::to_string(p++);
    EXPECT_EQ(seq, p - 1);
    EXPECT_EQ(cur_k, k);
    EXPECT_EQ(cur_v, v);
  }

  EXPECT_EQ(p, count);
}

TEST(oblsm_wal_test, DISABLED_oblsm_recover_with_small_amount_of_data)
{
  filesystem::remove_all("oblsm_tmp");
  filesystem::create_directory("oblsm_tmp");
  ObLsmOptions options;
  options.force_sync_new_log = false;
  ObLsm *lsm                 = nullptr;
  RC     rc                  = ObLsm::open(options, "oblsm_tmp", &lsm);
  EXPECT_EQ(rc, RC::SUCCESS);

  const int count = 32;
  for (int i = 0; i < count; ++i) {
    std::string key = "tmp_key" + to_string(i);
    lsm->put(key, key);
  }
  sleep(2);
  delete lsm;

  lsm = nullptr;
  ObLsm::open(options, "oblsm_tmp", &lsm);

  ObLsmReadOptions read_options;
  auto             iter = lsm->new_iterator(read_options);
  iter->seek_to_first();
  auto runner = 0;
  while (iter->valid()) {
    iter->next();
    runner++;
  }
  EXPECT_EQ(count, runner);
  delete iter;
  delete lsm;
}

TEST(oblsm_wal_test, DISABLED_oblsm_recover_with_single_thread)
{
  filesystem::remove_all("oblsm_tmp");
  filesystem::create_directory("oblsm_tmp");
  ObLsmOptions options;
  options.force_sync_new_log = false;
  ObLsm *lsm                 = nullptr;
  RC     rc                  = ObLsm::open(options, "oblsm_tmp", &lsm);
  EXPECT_EQ(rc, RC::SUCCESS);

  const int count = 90000;
  for (int i = 0; i < count; ++i) {
    std::string key = "tmp_key" + to_string(i);
    lsm->put(key, key);
  }
  sleep(2);
  delete lsm;

  lsm = nullptr;
  ObLsm::open(options, "oblsm_tmp", &lsm);

  ObLsmReadOptions read_options;
  auto             iter = lsm->new_iterator(read_options);
  iter->seek_to_first();
  auto runner = 0;
  while (iter->valid()) {
    iter->next();
    runner++;
  }
  EXPECT_EQ(count, runner);
  delete iter;
  delete lsm;
}

TEST(oblsm_wal_test, DISABLED_oblsm_recover_with_concurrent_put_no_sync)
{
  filesystem::remove_all("oblsm_tmp");
  filesystem::create_directory("oblsm_tmp");
  ObLsmOptions options;
  options.force_sync_new_log = false;

  ObLsm *lsm                 = nullptr;
  RC     rc                  = ObLsm::open(options, "oblsm_tmp", &lsm);
  EXPECT_EQ(rc, RC::SUCCESS);

  const int                kv_count     = 10000;
  const int                thread_count = 8;
  std::vector<std::thread> threads;
  for (auto i = 0; i < thread_count; ++i) {
    threads.emplace_back([i, lsm]() {
      for (auto j = 0; j < kv_count; ++j) {
        int         seq = i * kv_count + j;
        std::string k   = "key" + std::to_string(seq);
        std::string v   = "val" + std::to_string(seq);
        lsm->put(k, v);
      }
    });
  }

  for (auto& thread: threads) {
    thread.join();
  }

  sleep(2);
  delete lsm;

  lsm = nullptr;
  ObLsm::open(options, "oblsm_tmp", &lsm);

  ObLsmReadOptions read_options;
  auto             iter = lsm->new_iterator(read_options);
  iter->seek_to_first();
  auto runner = 0;
  while (iter->valid()) {
    iter->next();
    runner++;
  }
  EXPECT_EQ(kv_count * thread_count, runner);
  delete iter;
  delete lsm;
}

TEST(oblsm_wal_test, DISABLED_oblsm_recover_with_concurrent_put_sync)
{
  filesystem::remove_all("oblsm_tmp");
  filesystem::create_directory("oblsm_tmp");
  ObLsmOptions options;
  options.force_sync_new_log = true;

  ObLsm *lsm                 = nullptr;
  RC     rc                  = ObLsm::open(options, "oblsm_tmp", &lsm);
  EXPECT_EQ(rc, RC::SUCCESS);

  const int                kv_count     = 10000;
  const int                thread_count = 8;
  std::vector<std::thread> threads;
  for (auto i = 0; i < thread_count; ++i) {
    threads.emplace_back([i, lsm]() {
      for (auto j = 0; j < kv_count; ++j) {
        int         seq = i * kv_count + j;
        std::string k   = "key" + std::to_string(seq);
        std::string v   = "val" + std::to_string(seq);
        lsm->put(k, v);
      }
    });
  }

  for (auto& thread: threads) {
    thread.join();
  }

  sleep(2);
  delete lsm;

  lsm = nullptr;
  ObLsm::open(options, "oblsm_tmp", &lsm);

  ObLsmReadOptions read_options;
  auto             iter = lsm->new_iterator(read_options);
  iter->seek_to_first();
  auto runner = 0;
  while (iter->valid()) {
    iter->next();
    runner++;
  }
  EXPECT_EQ(kv_count * thread_count, runner);
  delete iter;
  delete lsm;
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}