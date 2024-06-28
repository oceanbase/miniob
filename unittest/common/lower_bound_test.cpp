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
// Created by Wangyunlai on 2022/9/5.
//

#include "common/lang/lower_bound.h"
#include "gtest/gtest.h"
#include "common/lang/vector.h"

using namespace common;

TEST(lower_bound, test_lower_bound)
{
  int         items[] = {1, 3, 5, 7, 9};
  const int   count   = sizeof(items) / sizeof(items[0]);
  vector<int> v(items, items + count);

  for (int i = 0; i <= 10; i++) {
    bool                  found  = false;
    vector<int>::iterator retval = lower_bound(v.begin(), v.end(), i, &found);
    ASSERT_EQ(retval - v.begin(), i / 2);
    ASSERT_EQ(found, (i % 2 != 0));
  }

  int         empty_items = {};
  const int   empty_count = 0;
  vector<int> empty_v(empty_items, empty_items + empty_count);
  for (int i = 0; i <= 10; i++) {
    bool                  found  = false;
    vector<int>::iterator retval = lower_bound(empty_v.begin(), empty_v.end(), i, &found);
    ASSERT_EQ(retval - empty_v.begin(), 0);
    ASSERT_EQ(found, false);
  }
}
int main(int argc, char **argv)
{
  // 分析gtest程序的命令行参数
  testing::InitGoogleTest(&argc, argv);

  // 调用RUN_ALL_TESTS()运行所有测试用例
  // main函数返回RUN_ALL_TESTS()的运行结果
  return RUN_ALL_TESTS();
}
