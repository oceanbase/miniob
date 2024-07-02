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
// Created by Wangyunlai on 2024/01/11.
//

#include "gtest/gtest.h"
#include "common/lang/memory.h"
#include "common/queue/simple_queue.h"

using namespace common;

TEST(SimpleQueue, test)
{
  SimpleQueue<int> queue;
  EXPECT_EQ(0, queue.size());

  int ret = queue.push(1);
  EXPECT_EQ(0, ret);
  EXPECT_EQ(1, queue.size());

  int value;
  ret = queue.pop(value);
  EXPECT_EQ(0, ret);
  EXPECT_EQ(1, value);
  EXPECT_EQ(0, queue.size());

  ret = queue.pop(value);
  EXPECT_EQ(-1, ret);
  EXPECT_EQ(0, queue.size());

  ret = queue.push(2);
  EXPECT_EQ(0, ret);
  EXPECT_EQ(1, queue.size());

  ret = queue.pop(value);
  EXPECT_EQ(0, ret);
  EXPECT_EQ(2, value);
  EXPECT_EQ(0, queue.size());
}

#if 0
TEST(SimpleQueue, test_big)
{
  SimpleQueue<int> queue;
  EXPECT_EQ(0, queue.size());

  for (int i = 0; i < 1000000; ++i) {
    int ret = queue.push(tmp);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(i + 1, queue.size());
  }

  for (int i = 0; i < 1000000; ++i) {
    int value;
    int ret = queue.pop(value);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(i, value);
    EXPECT_EQ(1000000 - i - 1, queue.size());
  }
}
#endif

TEST(SimpleQueue, test_unique_ptr)
{
  SimpleQueue<unique_ptr<int>> queue;
  EXPECT_EQ(0, queue.size());

  unique_ptr<int> ptr(new int(1));
  int             ret = queue.push(std::move(ptr));
  EXPECT_EQ(0, ret);
  EXPECT_EQ(1, queue.size());
  EXPECT_EQ(nullptr, ptr.get());

  unique_ptr<int> value;
  ret = queue.pop(value);
  EXPECT_EQ(0, ret);
  EXPECT_EQ(1, *value);
  EXPECT_EQ(0, queue.size());

  for (int i = 0; i < 1000000; ++i) {
    unique_ptr<int> ptr(new int(i));
    int             ret = queue.push(std::move(ptr));
    EXPECT_EQ(0, ret);
    EXPECT_EQ(i + 1, queue.size());
    EXPECT_EQ(nullptr, ptr.get());
  }
  // 不做pop，检查内存是否释放(ASAN会检查)
}

int main(int argc, char **argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}