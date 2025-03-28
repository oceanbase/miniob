/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "gtest/gtest.h"

#include "common/lang/filesystem.h"
#include "common/lang/thread.h"
#include "common/lang/utility.h"
#include "oblsm/include/ob_lsm.h"
#include "oblsm/ob_lsm_define.h"
#include "unittest/oblsm/ob_lsm_test_base.h"

using namespace oceanbase;

class ObLsmTest : public ObLsmTestBase {
};

// TODO: add update/delete case
TEST_P(ObLsmTest, DISABLED_oblsm_test_basic1)
{
  size_t num_entries = GetParam();
  auto data = KeyValueGenerator::generate_data(num_entries);

  for (const auto& [key, value] : data) {
    ASSERT_EQ(db->put(key, value), RC::SUCCESS);
  }

  for (const auto& [key, value] : data) {
    string fetched_value;
    ASSERT_EQ(db->get(key, &fetched_value), RC::SUCCESS);
    EXPECT_EQ(fetched_value, value);
  }

  ObLsmIterator* it = db->new_iterator(ObLsmReadOptions());
  it->seek_to_first();
  size_t count = 0;
  while (it->valid()) {
      it->next();
      ++count;
  }
  EXPECT_EQ(count, num_entries);
  delete it;

  ObLsmIterator* it2 = db->new_iterator(ObLsmReadOptions());
  it2->seek("key" + to_string(num_entries/2));
  ASSERT_TRUE(it2->valid());
  ASSERT_EQ(it2->value(), "value" + to_string(num_entries/2));
  while (it2->valid())
  {
    it2->next();
  }
  delete it2;  
}

void thread_put(ObLsm *db, int start, int end) {
  for (int i = start; i < end; ++i) {
    const std::string key = "key" + std::to_string(i);
    RC rc = db->put(key, key);
    ASSERT_EQ(rc, RC::SUCCESS);
  }
}

TEST_P(ObLsmTest, DISABLED_ConcurrentPutAndGetTest) {
  const int num_entries = GetParam();
  const int num_threads = 4;
  const int batch_size = num_entries / num_threads;

  std::vector<std::thread> threads;
  for (int i = 0; i < num_threads; ++i) {
    int start = i * batch_size;
    int end = 0;
    if (i == num_threads - 1) {
      end = num_entries;
    } else {
      end = start + batch_size;
    }
    threads.emplace_back(thread_put, db, start, end);
  }

  for (auto &thread : threads) {
    thread.join();
  }
  // Verify all data using iterator
  ObLsmReadOptions options;
  ObLsmIterator *iterator = db->new_iterator(options);

  iterator->seek_to_first();
  int count = 0;
  while (iterator->valid()) {
    iterator->next();
    ++count;
  }

  EXPECT_EQ(count, num_entries);

  // Clean up
  delete iterator;
}

TEST_P(ObLsmTest, DISABLED_ConcurrentPutAndRecoverTest) {
  const int num_entries = GetParam();
  const int num_threads = 4;
  const int batch_size = num_entries / num_threads;

  std::vector<std::thread> threads;
  for (int i = 0; i < num_threads; ++i) {
    int start = i * batch_size;
    int end = 0;
    if (i == num_threads - 1) {
      end = num_entries;
    } else {
      end = start + batch_size;
    }
    threads.emplace_back(thread_put, db, start, end);
  }

  for (auto &thread : threads) {
    thread.join();
  }
  // TODO: remove sleep
  sleep(2);

  // Close db
  delete db;
  ASSERT_EQ(ObLsm::open(this->options, this->path, &db), RC::SUCCESS);

  // Verify all data using iterator
  ObLsmReadOptions options;
  ObLsmIterator *iterator = db->new_iterator(options);

  iterator->seek_to_first();
  int count = 0;
  while (iterator->valid()) {
    iterator->next();
    ++count;
  }

  EXPECT_EQ(count, num_entries);

  // Clean up
  delete iterator;
}

INSTANTIATE_TEST_SUITE_P(
    ObLsmTests,
    ObLsmTest,
    ::testing::Values(1, 10, 1000, 10000, 50000)
);

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}