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
#include "oblsm/include/ob_lsm.h"
#include "oblsm/ob_lsm_impl.h"
#include "unittest/oblsm/ob_lsm_test_base.h"

using namespace oceanbase;

class ObLsmCompactionTest : public ObLsmTestBase {
};

bool check_compaction(ObLsm* lsm)
{
  ObLsmImpl *lsm_impl = dynamic_cast<ObLsmImpl*>(lsm);
  if (nullptr == lsm_impl) {
    return false;
  }
  auto sstables = lsm_impl->get_sstables();

  if (sstables->size() != ObLsmOptions().default_levels) {
    return false;
  }
  auto level_0 = sstables->at(0);
  if (level_0.size() > ObLsmOptions().default_l0_file_num) {
    return false;
  }

  ObLsmOptions options;

  // check level_i size
  size_t level_size = options.default_l1_level_size;
  for (size_t i = 1; i < options.default_levels; ++i) {
    const auto& level_i = sstables->at(i);
    int level_i_size = 0;
    for (const auto& sstable : level_i) {
      level_i_size += sstable->size();
    }
    if (level_i_size > level_size * 1.1) {
      return false;
    }
    level_size *= options.default_level_ratio;
  }

  // check level_i overlap
  for (size_t i = 1; i < options.default_levels; ++i) {
    const auto& level_i = sstables->at(i);
    vector<pair<string, string>> key_ranges;
    for (const auto& sstable : level_i) {
      key_ranges.push_back(make_pair(sstable->first_key(), sstable->last_key()));
    }
    ObInternalKeyComparator comp;
    std::sort(key_ranges.begin(), key_ranges.end(), [&](const auto& a, const auto& b) { return comp.compare(a.first, b.first) < 0; });
    for (size_t j = 1; j < key_ranges.size(); ++j) {
      if (comp.compare(key_ranges[j].first, key_ranges[j-1].second) < 0) {
        return false;
      }
    }
  }
  return true;
}

TEST_P(ObLsmCompactionTest, DISABLED_oblsm_compaction_test_basic1)
{
  size_t num_entries = GetParam();
  auto data = KeyValueGenerator::generate_data(num_entries);

  for (const auto& [key, value] : data) {
    ASSERT_EQ(db->put(key, value), RC::SUCCESS);
  }
  sleep(1);

  ObLsmIterator* it = db->new_iterator(ObLsmReadOptions());
  it->seek_to_first();
  size_t count = 0;
  while (it->valid()) {
      it->next();
      ++count;
  }
  EXPECT_EQ(count, num_entries);
  delete it;
  ASSERT_TRUE(check_compaction(db));
}

void thread_put(ObLsm *db, int start, int end) {
  for (int i = start; i < end; ++i) {
    const std::string key = "key" + std::to_string(i);
    RC rc = db->put(key, key);
    ASSERT_EQ(rc, RC::SUCCESS);
  }
}

TEST_P(ObLsmCompactionTest, DISABLED_ConcurrentPutAndGetTest) {
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
  // wait for compaction
  sleep(1);

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

  ASSERT_TRUE(check_compaction(db));
}

INSTANTIATE_TEST_SUITE_P(
    ObLsmCompactionTests,
    ObLsmCompactionTest,
    ::testing::Values(1, 10, 1000, 10000, 100000)
);

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}