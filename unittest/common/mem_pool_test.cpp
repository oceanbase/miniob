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
// Created by longda on 2022
//

#include "common/mm/mem_pool.h"
#include "gtest/gtest.h"
#include "common/lang/list.h"
#include "common/lang/iostream.h"

using namespace common;

TEST(test_mem_pool_item, test_mem_pool_item_basic)
{
  MemPoolItem mem_pool_item("test");

  const int item_num_per_pool = 128;
  mem_pool_item.init(32, true, 1, item_num_per_pool);
  list<void *> used_list;

  int alloc_num = 1000;

  for (int i = 0; i < alloc_num; i++) {
    void *item = mem_pool_item.alloc();
    used_list.push_back(item);
  }

  cout << mem_pool_item.to_string() << endl;

  int pool_size = ((alloc_num + item_num_per_pool - 1) / item_num_per_pool) * item_num_per_pool;
  ASSERT_EQ(alloc_num, mem_pool_item.get_used_num());
  ASSERT_EQ(pool_size, mem_pool_item.get_size());
  ASSERT_EQ(item_num_per_pool, mem_pool_item.get_item_num_per_pool());
  ASSERT_EQ(32, mem_pool_item.get_item_size());

  int free_num = item_num_per_pool * 3;
  for (int i = 0; i < free_num; i++) {
    auto item = used_list.front();
    used_list.pop_front();

    char *check = (char *)item + 10;
    mem_pool_item.free(item);
    mem_pool_item.free(check);
  }

  cout << mem_pool_item.to_string() << endl;
  ASSERT_EQ(used_list.size(), mem_pool_item.get_used_num());
  ASSERT_EQ(pool_size, mem_pool_item.get_size());
}

int main(int argc, char **argv)
{

  // 分析gtest程序的命令行参数
  testing::InitGoogleTest(&argc, argv);

  // 调用RUN_ALL_TESTS()运行所有测试用例
  // main函数返回RUN_ALL_TESTS()的运行结果
  return RUN_ALL_TESTS();
}