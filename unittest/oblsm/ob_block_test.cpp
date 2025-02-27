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

#include "oblsm/table/ob_block.h"
#include "oblsm/table/ob_block_builder.h"
#include "oblsm/util/ob_comparator.h"

using namespace oceanbase;

TEST(block_test, DISABLED_block_builder_test_basic)
{
  ObBlockBuilder builder;
  ObDefaultComparator comparator;
  builder.add("key1", "value1");
  builder.add("key2", "value2");
  builder.add("key3", "value3");
  ASSERT_EQ(builder.last_key(), "key3");
  builder.add("key4", "value4");
  ASSERT_EQ(builder.last_key(), "key4");
  string_view block_contents = builder.finish();

  ObBlock block(&comparator);
  block.decode(string(block_contents.data(), block_contents.size()));
  ASSERT_EQ(block.size(), 4);
}

TEST(block_test, DISABLED_block_iterator_test_basic)
{
  ObBlockBuilder builder;
  ObDefaultComparator comparator;
  builder.add("key1", "value1");
  builder.add("key2", "value2");
  builder.add("key3", "value3");
  ASSERT_EQ(builder.last_key(), "key3");
  builder.add("key4", "value4");
  ASSERT_EQ(builder.last_key(), "key4");
  string_view block_contents = builder.finish();

  ObBlock block(&comparator);
  block.decode(string(block_contents.data(), block_contents.size()));
  ASSERT_EQ(block.size(), 4);
  BlockIterator iter(&comparator, &block, block.size());
  iter.seek_to_first();
  ASSERT_TRUE(iter.valid());
  ASSERT_EQ(iter.key(), "key1");
  ASSERT_EQ(iter.value(), "value1");
  while(iter.valid()) {
    cout << iter.key() << " " << iter.value() << endl;
    iter.next();
  }
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}