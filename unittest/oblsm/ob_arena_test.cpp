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

#include "oblsm/util/ob_arena.h"
#include "common/math/random_generator.h"

using namespace oceanbase;

TEST(arena_test, DISABLED_arena_test_basic)
{
  ObArena arena;
  const int count = 1000;
  size_t bytes = 0;
  common::RandomGenerator rnd;
  for (int i = 0; i < count; i++) {
    size_t s;
    s = rnd.next(4000);
    if (s == 0) {
      s = 1;
    }
    char* r;
    r = arena.alloc(s);

    for (size_t b = 0; b < s; b++) {
      r[b] = i % 256;
    }
    bytes += s;
    bytes += sizeof(char*);
    ASSERT_EQ(arena.memory_usage(), bytes);
  }
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}