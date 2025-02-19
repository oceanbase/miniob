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

#include "common/lang/string.h"
#include "common/lang/vector.h"
#include "common/lang/thread.h"
#include "common/lang/utility.h"
#include "oblsm/util/ob_lru_cache.h"

using namespace oceanbase;

class ObLRUCacheTest : public ::testing::TestWithParam<size_t> {
protected:
  ObLRUCache<string, string>* cache;
  size_t capacity;

  void SetUp() override {
    capacity = GetParam();
    cache = new_lru_cache<string, string>(capacity);
  }

  void TearDown() override {
    delete cache;
  }
};

TEST_P(ObLRUCacheTest, DISABLED_lru_capacity) {
  ASSERT_NE(cache, nullptr);

  for (size_t i = 0; i < capacity + 2; ++i) {
    string key = "key" + to_string(i);
    string value = "value" + to_string(i);
    cache->put(key, value);
  }

  for (size_t i = 0; i < capacity + 2; ++i) {
    string key = "key" + to_string(i);
    string value;
    if (i < 2) {
      EXPECT_FALSE(cache->get(key, value));
    } else {
      EXPECT_TRUE(cache->get(key, value));
      EXPECT_EQ(value, "value" + to_string(i));
    }
  }
}

TEST_P(ObLRUCacheTest, DISABLED_update_exist_key) {
  ASSERT_NE(cache, nullptr);

  cache->put("key1", "value1");
  cache->put("key2", "value2");
  cache->put("key1", "value1_updated");

  string value;

  EXPECT_TRUE(cache->get("key1", value));
  EXPECT_EQ(value, "value1_updated");

  EXPECT_TRUE(cache->get("key2", value));
  EXPECT_EQ(value, "value2");
}

TEST_P(ObLRUCacheTest, DISABLED_contains_key) {
    ASSERT_NE(cache, nullptr);

    cache->put("key1", "value1");
    cache->put("key2", "value2");

    EXPECT_TRUE(cache->contains("key1"));
    EXPECT_TRUE(cache->contains("key2"));
    EXPECT_FALSE(cache->contains("key3"));

    string value;
    EXPECT_TRUE(cache->get("key1", value));
    EXPECT_EQ(value, "value1");
}

INSTANTIATE_TEST_SUITE_P(
  CacheTests,
  ObLRUCacheTest,
  ::testing::Values(2, 5, 10, 100, 10000)
);

TEST(lru_test, zero_capacity)
{
  ObLRUCache<int, std::string> lru_cache(0);
  lru_cache.put(1, "one");
  ASSERT_FALSE(lru_cache.contains(1));
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}