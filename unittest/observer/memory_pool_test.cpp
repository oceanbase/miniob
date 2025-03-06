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
// Created by Ping Xu (haibarapink@gmail.com)
//
#include "gtest/gtest.h"
#include "common/mm/mem_pool.h"

struct Frame
{
  char buf[4096];

  void reinit() {}
  void reset() {}
};

// PROCESS WILL CRASH!
TEST(mm, DISABLED_mm_illegal_access)
{
  using namespace common;
  MemPoolSimple<Frame> pool{"mm_illegal_access"};
  ASSERT_TRUE(pool.init() == 0);
  auto frame = pool.alloc();

  // Free frame
  pool.free(frame);

  // Access frame. Process WILL CRASH!
  auto buf = frame->buf;
  buf[0]   = '1';
}

TEST(mm, mm_legal_access)
{
  using namespace common;
  MemPoolSimple<Frame> pool{"mm_illegal_access"};
  ASSERT_TRUE(pool.init(false, 3, 3) == 0);
  std::vector<Frame *> frames;

  for (auto i = 0; i < 3; ++i) {
    frames.push_back(pool.alloc());
  }

  for (auto i = 0; i < 3; ++i) {
    pool.free(frames.at(i));
  }

  frames.clear();
  for (auto i = 0; i < 3; ++i) {
    Frame* f = pool.alloc();
    f->buf[0] = 1;
    pool.free(f);
  }
}

int main(int argc, char **argv)
{

  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
