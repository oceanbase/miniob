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
// Created by wangyunlai.wyl on 2023/08/14
//

#include <memory>

#include "sql/expr/expression.h"
#include "gtest/gtest.h"

using namespace std;
using namespace common;

TEST(ArithmeticExpr, test_value_type)
{
  Value int_value1(1);
  Value int_value2(2);
  Value float_value1((float)1.1);
  Value float_value2((float)2.2);

  unique_ptr<Expression> left_expr(new ValueExpr(int_value1));
  unique_ptr<Expression> right_expr(new ValueExpr(int_value2));
  ArithmeticExpr expr_int(ArithmeticExpr::Type::ADD, std::move(left_expr), std::move(right_expr));
  ASSERT_EQ(AttrType::INTS, expr_int.value_type());

  left_expr.reset(new ValueExpr(float_value1));
  right_expr.reset(new ValueExpr(float_value2));
  ArithmeticExpr expr_float(ArithmeticExpr::Type::ADD, std::move(left_expr), std::move(right_expr));
  ASSERT_EQ(AttrType::FLOATS, expr_float.value_type());

  left_expr.reset(new ValueExpr(int_value1));
  right_expr.reset(new ValueExpr(float_value2));
  ArithmeticExpr expr_int_float(ArithmeticExpr::Type::ADD, std::move(left_expr), std::move(right_expr));
  ASSERT_EQ(AttrType::FLOATS, expr_int_float.value_type());

  left_expr.reset(new ValueExpr(int_value1));
  right_expr.reset(new ValueExpr(int_value2));
  ArithmeticExpr expr_int_int(ArithmeticExpr::Type::DIV, std::move(left_expr), std::move(right_expr));
  ASSERT_EQ(AttrType::FLOATS, expr_int_int.value_type());
}

TEST(ArithmeticExpr, test_try_get_value)
{
  Value int_value1(1);
  Value int_value2(2);
  Value float_value1((float)1.1);
  Value float_value2((float)2.2);

  Value int_result;
  Value float_result;

  unique_ptr<ValueExpr> left_expr(new ValueExpr(int_value1));
  unique_ptr<ValueExpr> right_expr(new ValueExpr(int_value2));
  ArithmeticExpr int_expr(ArithmeticExpr::Type::ADD, std::move(left_expr), std::move(right_expr));
  
  ASSERT_EQ(int_expr.try_get_value(int_result), RC::SUCCESS);
  ASSERT_EQ(int_result.get_int(), 3);

  int_expr.~ArithmeticExpr();

  left_expr.reset(new ValueExpr(int_value1));
  right_expr.reset(new ValueExpr(int_value2));
  new (&int_expr)(ArithmeticExpr)(ArithmeticExpr::Type::SUB, std::move(left_expr), std::move(right_expr));
  ASSERT_EQ(int_expr.try_get_value(int_result), RC::SUCCESS);
  ASSERT_EQ(int_result.get_int(), -1);

  left_expr.reset(new ValueExpr(int_value1));
  right_expr.reset(new ValueExpr(int_value2));
  int_expr.~ArithmeticExpr();
  new (&int_expr)(ArithmeticExpr)(ArithmeticExpr::Type::MUL, std::move(left_expr), std::move(right_expr));
  ASSERT_EQ(int_expr.try_get_value(int_result), RC::SUCCESS);
  ASSERT_EQ(int_result.get_int(), 2);

  left_expr.reset(new ValueExpr(int_value1));
  right_expr.reset(new ValueExpr(int_value2));
  int_expr.~ArithmeticExpr();
  new (&int_expr)(ArithmeticExpr)(ArithmeticExpr::Type::DIV, std::move(left_expr), std::move(right_expr));
  ASSERT_EQ(int_expr.try_get_value(float_result), RC::SUCCESS);
  EXPECT_FLOAT_EQ(float_result.get_float(), 0.5);

  left_expr.reset(new ValueExpr(float_value1));
  right_expr.reset(new ValueExpr(float_value2));
  ArithmeticExpr float_expr(ArithmeticExpr::Type::ADD, std::move(left_expr), std::move(right_expr));
  ASSERT_EQ(float_expr.try_get_value(float_result), RC::SUCCESS);
  EXPECT_FLOAT_EQ(float_result.get_float(), 3.3);

  left_expr.reset(new ValueExpr(float_value1));
  right_expr.reset(new ValueExpr(float_value2));
  float_expr.~ArithmeticExpr();
  new (&float_expr)(ArithmeticExpr)(ArithmeticExpr::Type::SUB, std::move(left_expr), std::move(right_expr));
  ASSERT_EQ(float_expr.try_get_value(float_result), RC::SUCCESS);
  EXPECT_FLOAT_EQ(float_result.get_float(), -1.1);

  left_expr.reset(new ValueExpr(float_value1));
  right_expr.reset(new ValueExpr(float_value2));
  float_expr.~ArithmeticExpr();
  new (&float_expr)(ArithmeticExpr)(ArithmeticExpr::Type::MUL, std::move(left_expr), std::move(right_expr));
  ASSERT_EQ(float_expr.try_get_value(float_result), RC::SUCCESS);
  EXPECT_FLOAT_EQ(float_result.get_float(), 1.1*2.2);

  left_expr.reset(new ValueExpr(float_value1));
  right_expr.reset(new ValueExpr(float_value2));
  float_expr.~ArithmeticExpr();
  new (&float_expr)(ArithmeticExpr)(ArithmeticExpr::Type::DIV, std::move(left_expr), std::move(right_expr));
  ASSERT_EQ(float_expr.try_get_value(float_result), RC::SUCCESS);
  EXPECT_FLOAT_EQ(float_result.get_float(), 1.1/2.2);

  Value zero_int_value(0);
  Value zero_float_value((float)0);
  left_expr.reset(new ValueExpr(int_value1));
  right_expr.reset(new ValueExpr(zero_int_value));
  ArithmeticExpr zero_int_expr(ArithmeticExpr::Type::DIV, std::move(left_expr), std::move(right_expr));
  ASSERT_EQ(zero_int_expr.try_get_value(float_result), RC::SUCCESS);

  left_expr.reset(new ValueExpr(float_value1));
  right_expr.reset(new ValueExpr(zero_float_value));
  ArithmeticExpr zero_float_expr(ArithmeticExpr::Type::DIV, std::move(left_expr), std::move(right_expr));
  ASSERT_EQ(zero_float_expr.try_get_value(float_result), RC::SUCCESS);

  left_expr.reset(new ValueExpr(int_value1));
  ArithmeticExpr negative_expr(ArithmeticExpr::Type::NEGATIVE, std::move(left_expr), nullptr);
  ASSERT_EQ(negative_expr.try_get_value(int_result), RC::SUCCESS);
  ASSERT_EQ(int_result.get_int(), -1);

  left_expr.reset(new ValueExpr(float_value1));
  ArithmeticExpr negative_float_expr(ArithmeticExpr::Type::NEGATIVE, std::move(left_expr), nullptr);
  ASSERT_EQ(negative_float_expr.try_get_value(float_result), RC::SUCCESS);
  EXPECT_FLOAT_EQ(float_result.get_float(), -1.1);
}

int main(int argc, char **argv)
{

  // 分析gtest程序的命令行参数
  testing::InitGoogleTest(&argc, argv);

  // 调用RUN_ALL_TESTS()运行所有测试用例
  // main函数返回RUN_ALL_TESTS()的运行结果
  return RUN_ALL_TESTS();
}
