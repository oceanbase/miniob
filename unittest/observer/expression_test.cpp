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
#include "sql/expr/tuple.h"
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
  ArithmeticExpr         expr_int(ArithmeticExpr::Type::ADD, std::move(left_expr), std::move(right_expr));
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
  ArithmeticExpr        int_expr(ArithmeticExpr::Type::ADD, std::move(left_expr), std::move(right_expr));

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
  EXPECT_FLOAT_EQ(float_result.get_float(), 1.1 * 2.2);

  left_expr.reset(new ValueExpr(float_value1));
  right_expr.reset(new ValueExpr(float_value2));
  float_expr.~ArithmeticExpr();
  new (&float_expr)(ArithmeticExpr)(ArithmeticExpr::Type::DIV, std::move(left_expr), std::move(right_expr));
  ASSERT_EQ(float_expr.try_get_value(float_result), RC::SUCCESS);
  EXPECT_FLOAT_EQ(float_result.get_float(), 1.1 / 2.2);

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

TEST(ArithmeticExpr, get_column)
{
  // constant value
  {
    Value int_value1(1);
    Value int_value2(2);
    Value float_value1((float)1.1);
    Value float_value2((float)2.2);
    Chunk chunk;

    Value int_result;
    Value float_result;

    unique_ptr<ValueExpr> left_expr(new ValueExpr(int_value1));
    unique_ptr<ValueExpr> right_expr(new ValueExpr(int_value2));
    ArithmeticExpr        int_expr(ArithmeticExpr::Type::ADD, std::move(left_expr), std::move(right_expr));
    Column                column;
    ASSERT_EQ(int_expr.get_column(chunk, column), RC::SUCCESS);
    ASSERT_EQ(column.count(), 1);
    ASSERT_EQ(column.get_value(0).get_int(), 3);

    left_expr.reset(new ValueExpr(int_value1));
    right_expr.reset(new ValueExpr(int_value2));
    int_expr.~ArithmeticExpr();
    column.reset_data();
    new (&int_expr)(ArithmeticExpr)(ArithmeticExpr::Type::ADD, std::move(left_expr), std::move(right_expr));
    ASSERT_EQ(int_expr.get_column(chunk, column), RC::SUCCESS);
    ASSERT_EQ(column.count(), 1);
    ASSERT_EQ(column.get_value(0).get_int(), 3);

    left_expr.reset(new ValueExpr(float_value1));
    right_expr.reset(new ValueExpr(float_value2));
    ArithmeticExpr float_expr(ArithmeticExpr::Type::ADD, std::move(left_expr), std::move(right_expr));
    ASSERT_EQ(float_expr.get_column(chunk, column), RC::SUCCESS);
    ASSERT_EQ(column.count(), 1);
    EXPECT_FLOAT_EQ(column.get_value(0).get_float(), 3.3);
  }

  // column op constant value
  {
    int                     count       = 8;
    const int               int_len     = sizeof(int);
    std::unique_ptr<Column> column_left = std::make_unique<Column>(AttrType::FLOATS, int_len, count);
    Value                   float_value(2.0f);
    unique_ptr<ValueExpr>   right_expr(new ValueExpr(float_value));
    for (int i = 0; i < count; ++i) {
      float left_value = i;
      column_left->append_one((char *)&left_value);
    }
    Chunk chunk;
    chunk.add_column(std::move(column_left), 0);
    Column         column_result;
    FieldMeta      field_meta1("col1", AttrType::INTS, 0, int_len, true, 0);
    Field          field1(nullptr, &field_meta1);
    auto           left_expr = std::make_unique<FieldExpr>(field1);
    ArithmeticExpr expr(ArithmeticExpr::Type::DIV, std::move(left_expr), std::move(right_expr));
    ASSERT_EQ(expr.get_column(chunk, column_result), RC::SUCCESS);
    for (int i = 0; i < count; ++i) {
      float expect_value = static_cast<float>(i) / 2.0f;
      ASSERT_EQ(column_result.get_value(i).get_float(), expect_value);
    }
  }

  // column op column
  {
    int                     count        = 8;
    const int               int_len      = sizeof(int);
    std::unique_ptr<Column> column_left  = std::make_unique<Column>(AttrType::INTS, int_len, count);
    std::unique_ptr<Column> column_right = std::make_unique<Column>(AttrType::INTS, int_len, count);
    for (int i = 0; i < count; ++i) {
      int left_value  = i;
      int right_value = i;
      column_left->append_one((char *)&left_value);
      column_right->append_one((char *)&right_value);
    }
    Chunk chunk;
    chunk.add_column(std::move(column_left), 0);
    chunk.add_column(std::move(column_right), 1);
    Column         column_result;
    FieldMeta      field_meta1("col1", AttrType::INTS, 0, int_len, true, 0);
    FieldMeta      field_meta2("col2", AttrType::INTS, 0, int_len, true, 1);
    Field          field1(nullptr, &field_meta1);
    Field          field2(nullptr, &field_meta2);
    auto           left_expr  = std::make_unique<FieldExpr>(field1);
    auto           right_expr = std::make_unique<FieldExpr>(field2);
    ArithmeticExpr expr(ArithmeticExpr::Type::ADD, std::move(left_expr), std::move(right_expr));
    ASSERT_EQ(expr.get_column(chunk, column_result), RC::SUCCESS);
    for (int i = 0; i < count; ++i) {
      int expect_value = i * 2;
      ASSERT_EQ(column_result.get_value(i).get_int(), expect_value);
    }
  }
}

TEST(ValueExpr, value_expr_test)
{
  ValueExpr value_expr1(Value(1));
  ValueExpr value_expr2(Value(2));
  Value     result;
  value_expr1.get_value(result);
  ASSERT_EQ(result.get_int(), 1);
  ASSERT_EQ(value_expr1.equal(value_expr2), false);
  ASSERT_EQ(value_expr1.equal(value_expr1), true);
}

TEST(CastExpr, cast_expr_test)
{
  Value                  int_value1(1);
  unique_ptr<Expression> value_expr1 = make_unique<ValueExpr>(int_value1);
  CastExpr               cast_expr(std::move(value_expr1), AttrType::INTS);
  RowTuple               tuple;
  Value                  result;
  cast_expr.get_value(tuple, result);
  ASSERT_EQ(result.get_int(), 1);
  result.reset();
  cast_expr.try_get_value(result);
  ASSERT_EQ(result.get_int(), 1);
}

TEST(ComparisonExpr, comparison_expr_test)
{
  {
    Value int_value1(1);
    Value int_value2(2);
    bool  bool_res = false;
    Value bool_value;

    unique_ptr<Expression> left_expr(new ValueExpr(int_value1));
    unique_ptr<Expression> right_expr(new ValueExpr(int_value2));
    ComparisonExpr         expr_eq(CompOp::EQUAL_TO, std::move(left_expr), std::move(right_expr));
    ASSERT_EQ(AttrType::BOOLEANS, expr_eq.value_type());
    ASSERT_EQ(expr_eq.compare_value(int_value1, int_value2, bool_res), RC::SUCCESS);
    ASSERT_EQ(bool_res, false);
    ASSERT_EQ(expr_eq.compare_value(int_value1, int_value1, bool_res), RC::SUCCESS);
    ASSERT_EQ(bool_res, true);
    ASSERT_EQ(expr_eq.try_get_value(bool_value), RC::SUCCESS);
    ASSERT_EQ(bool_value.get_boolean(), false);
  }
  {
    const int               int_len = sizeof(int);
    Value                   int_value(1);
    FieldMeta               field_meta("col1", AttrType::INTS, 0, int_len, true, 0);
    Field                   field(nullptr, &field_meta);
    unique_ptr<Expression>  right_expr  = std::make_unique<FieldExpr>(field);
    int                     count       = 1024;
    std::unique_ptr<Column> column_right = std::make_unique<Column>(AttrType::INTS, int_len, count);
    for (int i = 0; i < count; ++i) {
      int right_value = i;
      column_right->append_one((char *)&right_value);
    }
    Chunk                chunk;
    std::vector<uint8_t> select(count, 1);
    chunk.add_column(std::move(column_right), 0);

    unique_ptr<Expression> left_expr(new ValueExpr(int_value));
    ComparisonExpr         expr_eq(CompOp::EQUAL_TO, std::move(left_expr), std::move(right_expr));
    ASSERT_EQ(AttrType::BOOLEANS, expr_eq.value_type());
    RC rc = expr_eq.eval(chunk, select);
    ASSERT_EQ(rc, RC::SUCCESS);
    for (int i = 0; i < count; ++i) {
      if (i == 1) {
        ASSERT_EQ(select[i], 1);
      } else {
        ASSERT_EQ(select[i], 0);
      }
    }
  }
}

TEST(AggregateExpr, aggregate_expr_test)
{
  Value                  int_value(1);
  unique_ptr<Expression> value_expr(new ValueExpr(int_value));
  AggregateExpr          aggregate_expr(AggregateExpr::Type::SUM, std::move(value_expr));
  aggregate_expr.equal(aggregate_expr);
  auto aggregator = aggregate_expr.create_aggregator();
  for (int i = 0; i < 100; i++) {
    aggregator->accumulate(Value(i));
  }
  Value result;
  aggregator->evaluate(result);
  ASSERT_EQ(result.get_int(), 4950);
  AggregateExpr::Type aggr_type;
  ASSERT_EQ(RC::SUCCESS, AggregateExpr::type_from_string("sum", aggr_type));
  ASSERT_EQ(aggr_type, AggregateExpr::Type::SUM);
  ASSERT_EQ(RC::SUCCESS, AggregateExpr::type_from_string("count", aggr_type));
  ASSERT_EQ(aggr_type, AggregateExpr::Type::COUNT);
  ASSERT_EQ(RC::SUCCESS, AggregateExpr::type_from_string("avg", aggr_type));
  ASSERT_EQ(aggr_type, AggregateExpr::Type::AVG);
  ASSERT_EQ(RC::SUCCESS, AggregateExpr::type_from_string("max", aggr_type));
  ASSERT_EQ(aggr_type, AggregateExpr::Type::MAX);
  ASSERT_EQ(RC::SUCCESS, AggregateExpr::type_from_string("min", aggr_type));
  ASSERT_EQ(aggr_type, AggregateExpr::Type::MIN);
  ASSERT_EQ(RC::INVALID_ARGUMENT, AggregateExpr::type_from_string("invalid type", aggr_type));
}

int main(int argc, char **argv)
{

  // 分析gtest程序的命令行参数
  testing::InitGoogleTest(&argc, argv);

  // 调用RUN_ALL_TESTS()运行所有测试用例
  // main函数返回RUN_ALL_TESTS()的运行结果
  return RUN_ALL_TESTS();
}
