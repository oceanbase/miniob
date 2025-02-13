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

#include <thread>
#include "oblsm/util/ob_bloomfilter.h"

using namespace oceanbase;

TEST(BloomfilterTest, DISABLED_ConstructorTest) {
    ObBloomfilter bf(4);
    EXPECT_TRUE(bf.empty());
    EXPECT_EQ(bf.object_count(), 0);
}

TEST(BloomfilterTest, DISABLED_InsertAndContainsTest) {
    ObBloomfilter bf(4);

    bf.insert("database");
    bf.insert("db");

    EXPECT_TRUE(bf.contains("database"));
    EXPECT_TRUE(bf.contains("db"));
    EXPECT_FALSE(bf.contains("oceanbase"));

    EXPECT_EQ(bf.object_count(), 2);
}

TEST(BloomfilterTest, DISABLED_ClearTest) {
    ObBloomfilter bf(4);

    bf.insert("bloom");
    bf.insert("filter");
    bf.clear();

    EXPECT_FALSE(bf.contains("bloom"));
    EXPECT_FALSE(bf.contains("filter"));
    EXPECT_TRUE(bf.empty());
    EXPECT_EQ(bf.object_count(), 0);
}

TEST(BloomfilterTest, DISABLED_EmptyTest) {
    ObBloomfilter bf(4);

    EXPECT_TRUE(bf.empty());

    bf.insert("ob");
    EXPECT_FALSE(bf.empty());

    bf.clear();
    EXPECT_TRUE(bf.empty());
}

TEST(BloomFilterTest, DISABLED_MultiThreadInsertTest) {
    ObBloomfilter bloom_filter;
    const size_t thread_count = 10;
    const size_t insertions_per_thread = 1000;

    auto insert_function = [&bloom_filter](size_t id) {
        for (size_t i = 0; i < insertions_per_thread; ++i) {
            bloom_filter.insert("item_" + std::to_string(id) + "_" + std::to_string(i));
        }
    };

    std::vector<std::thread> threads;
    for (size_t i = 0; i < thread_count; ++i) {
        threads.emplace_back(insert_function, i);
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(bloom_filter.object_count(), thread_count * insertions_per_thread);

    EXPECT_TRUE(bloom_filter.contains("item_1_500"));
    EXPECT_FALSE(bloom_filter.contains("non_existent_item"));
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}