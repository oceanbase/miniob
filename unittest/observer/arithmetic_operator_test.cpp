/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include <memory>

#include "sql/expr/arithmetic_operator.hpp"
#include "gtest/gtest.h"

using namespace std;

TEST(ArithmeticTest, arithmetic_test)
{
  // compare column
  {
    int                  size = 100;
    std::vector<int>     a(size, 1);
    std::vector<int>     b(size, 2);
    std::vector<uint8_t> result(size, 1);
    compare_result<int, false, false>(a.data(), b.data(), size, result, CompOp::EQUAL_TO);
    for (int i = 0; i < size; ++i) {
      ASSERT_EQ(result[i], 0);
    }
  }
  // addition
  {
    int              size = 100;
    std::vector<int> a(size, 1);
    std::vector<int> b(size, 2);
    std::vector<int> result(size, 0);
    binary_operator<false, false, int, AddOperator>(a.data(), b.data(), result.data(), size);
    for (int i = 0; i < size; ++i) {
      ASSERT_EQ(result[i], 3);
    }
  }
  // subtraction
  {
    int              size = 100;
    std::vector<int> a(size, 1);
    std::vector<int> b(size, 2);
    std::vector<int> result(size, 0);
    binary_operator<false, false, int, SubtractOperator>(a.data(), b.data(), result.data(), size);
    for (int i = 0; i < size; ++i) {
      ASSERT_EQ(result[i], -1);
    }
  }
  // multiplication
  {
    int              size = 100;
    std::vector<int> a(size, 1);
    std::vector<int> b(size, 2);
    std::vector<int> result(size, 0);
    binary_operator<false, false, int, MultiplyOperator>(a.data(), b.data(), result.data(), size);
    for (int i = 0; i < size; ++i) {
      ASSERT_EQ(result[i], 2);
    }
  }
  // division
  {
    int              size = 100;
    std::vector<int> a(size, 2);
    std::vector<int> b(size, 2);
    std::vector<int> result(size, 0);
    binary_operator<false, false, int, DivideOperator>(a.data(), b.data(), result.data(), size);
    for (int i = 0; i < size; ++i) {
      ASSERT_EQ(result[i], 1);
    }
  }
  // negate
  {
    int              size = 100;
    std::vector<int> a(size, 1);
    std::vector<int> b(size, 2);
    std::vector<int> result(size, 0);
    unary_operator<false, int, NegateOperator>(a.data(), result.data(), size);
    for (int i = 0; i < size; ++i) {
      ASSERT_EQ(result[i], -1);
    }
  }
// sum
#ifdef USE_SIMD
  {
    int              size = 100;
    std::vector<int> a(size, 0);
    for (int i = 0; i < size; i++) {
      a[i] = i;
    }
    int res = mm256_sum_epi32(a.data(), size);
    ASSERT_EQ(res, 4950);
  }
  {
    int                size = 100;
    std::vector<float> a(size, 0);
    for (int i = 0; i < size; i++) {
      a[i] = i;
    }
    int res = mm256_sum_ps(a.data(), size);
    ASSERT_FLOAT_EQ(res, 4950.0);
  }
#endif
}

int main(int argc, char **argv)
{

  // 分析gtest程序的命令行参数
  testing::InitGoogleTest(&argc, argv);

  // 调用RUN_ALL_TESTS()运行所有测试用例
  // main函数返回RUN_ALL_TESTS()的运行结果
  return RUN_ALL_TESTS();
}
