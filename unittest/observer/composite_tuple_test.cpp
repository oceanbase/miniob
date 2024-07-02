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
// Created by wangyunlai.wyl on 2024/05/31
//

#include <memory>

#include "sql/expr/composite_tuple.h"
#include "gtest/gtest.h"

using namespace std;
using namespace common;

TEST(CompositeTuple, test_empty)
{
  CompositeTuple tuple1;
  ASSERT_EQ(0, tuple1.cell_num());

  Value value;
  TupleCellSpec spec;
  ASSERT_NE(RC::SUCCESS, tuple1.cell_at(0, value));
  ASSERT_NE(RC::SUCCESS, tuple1.spec_at(0, spec));

  ValueListTuple value_list_tuple;

  tuple1.add_tuple(make_unique<ValueListTuple>(value_list_tuple));
  ASSERT_EQ(0, tuple1.cell_num());

  ValueListTuple value_list_tuple2;
  tuple1.add_tuple(make_unique<ValueListTuple>(value_list_tuple2));
  ASSERT_EQ(0, tuple1.cell_num());

}

TEST(CompositeTuple, test_1tuple)
{
  // 测试包含一个tuple
  CompositeTuple composite_tuple;

  ValueListTuple value_list_tuple;
  vector<Value> values;
  vector<TupleCellSpec> specs;
  for (int i = 0; i < 10; i++)
  {
    values.emplace_back(i);
    specs.emplace_back(TupleCellSpec(to_string(i)));
  }

  value_list_tuple.set_cells(values);
  value_list_tuple.set_names(specs);

  composite_tuple.add_tuple(make_unique<ValueListTuple>(value_list_tuple));
  ASSERT_EQ(10, composite_tuple.cell_num());

  for (int i = 0; i < 10; i++) {
    Value value;
    TupleCellSpec spec;
    ASSERT_EQ(RC::SUCCESS, composite_tuple.cell_at(i, value));
    ASSERT_EQ(RC::SUCCESS, composite_tuple.spec_at(i, spec));
    ASSERT_EQ(i, value.get_int());
    ASSERT_EQ(to_string(i), spec.alias());
  }

  Value value;
  TupleCellSpec spec;
  ASSERT_NE(RC::SUCCESS, composite_tuple.cell_at(10, value));
  ASSERT_NE(RC::SUCCESS, composite_tuple.spec_at(10, spec));
  ASSERT_NE(RC::SUCCESS, composite_tuple.cell_at(11, value));
  ASSERT_NE(RC::SUCCESS, composite_tuple.spec_at(11, spec));
}

TEST(CompositeTuple, test_2tuple)
{
  // 测试包含两个tuple
  CompositeTuple composite_tuple;
  vector<Value> values;
  vector<TupleCellSpec> specs;
  for (int i = 0; i < 10; i++)
  {
    values.emplace_back(i);
    specs.emplace_back(TupleCellSpec(to_string(i)));
  }

  ValueListTuple value_list_tuple;
  value_list_tuple.set_cells(values);
  value_list_tuple.set_names(specs);

  composite_tuple.add_tuple(make_unique<ValueListTuple>(value_list_tuple));
  ASSERT_EQ(10, composite_tuple.cell_num());

  composite_tuple.add_tuple(make_unique<ValueListTuple>(value_list_tuple));
  ASSERT_EQ(20, composite_tuple.cell_num());

  for (int i = 0; i < 10; i++) {
    Value value;
    TupleCellSpec spec;
    ASSERT_EQ(RC::SUCCESS, composite_tuple.cell_at(i, value));
    ASSERT_EQ(RC::SUCCESS, composite_tuple.spec_at(i, spec));
    ASSERT_EQ(i, value.get_int());
    ASSERT_EQ(to_string(i), spec.alias());
  }

  for (int i = 0; i < 10; i++) {
    Value value;
    TupleCellSpec spec;
    ASSERT_EQ(RC::SUCCESS, composite_tuple.cell_at(i + 10, value));
    ASSERT_EQ(RC::SUCCESS, composite_tuple.spec_at(i + 10, spec));
    ASSERT_EQ(i, value.get_int());
    ASSERT_EQ(to_string(i), spec.alias());
  }

  Value value;
  TupleCellSpec spec;
  ASSERT_NE(RC::SUCCESS, composite_tuple.cell_at(20, value));
  ASSERT_NE(RC::SUCCESS, composite_tuple.spec_at(20, spec));
  ASSERT_NE(RC::SUCCESS, composite_tuple.cell_at(21, value));
  ASSERT_NE(RC::SUCCESS, composite_tuple.spec_at(21, spec));
}

TEST(CompositeTuple, test_including_composite_tuple)
{
  // 测试包含一个 composite tuple
  CompositeTuple composite_tuple;
  vector<Value> values;
  vector<TupleCellSpec> specs;
  for (int i = 0; i < 10; i++)
  {
    values.emplace_back(i);
    specs.emplace_back(TupleCellSpec(to_string(i)));
  }

  ValueListTuple value_list_tuple;
  value_list_tuple.set_cells(values);
  value_list_tuple.set_names(specs);

  composite_tuple.add_tuple(make_unique<ValueListTuple>(value_list_tuple));
  ASSERT_EQ(10, composite_tuple.cell_num());

  CompositeTuple composite_tuple2;
  composite_tuple2.add_tuple(make_unique<CompositeTuple>(std::move(composite_tuple)));
  ASSERT_EQ(10, composite_tuple2.cell_num());
}

int main(int argc, char **argv)
{

  // 分析gtest程序的命令行参数
  testing::InitGoogleTest(&argc, argv);

  // 调用RUN_ALL_TESTS()运行所有测试用例
  // main函数返回RUN_ALL_TESTS()的运行结果
  return RUN_ALL_TESTS();
}
