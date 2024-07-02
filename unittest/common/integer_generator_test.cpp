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
// Created by wangyunlai.wyl on 2024/01/15
//

#include "gtest/gtest.h"
#include "common/math/integer_generator.h"

using namespace common;

TEST(IntegerGenerator, test)
{
  const int        min = 1;
  const int        max = 120000;
  IntegerGenerator generator(min, max);
  for (int i = 0; i < 1000000; i++) {
    int value = generator.next();
    ASSERT_GE(value, min);
    ASSERT_LE(value, max);
  }
}

int main(int argc, char **argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
