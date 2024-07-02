/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include <chrono>
#include <iostream>

#include "gtest/gtest.h"
#include "sql/expr/aggregate_hash_table.h"

using namespace std;

TEST(AggregateHashTableTest, DISABLED_standard_hash_table)
{
  // single group by column, single aggregate column
  {
    Chunk                   group_chunk;
    Chunk                   aggr_chunk;
    std::unique_ptr<Column> column1 = std::make_unique<Column>(AttrType::INTS, 4);
    std::unique_ptr<Column> column2 = std::make_unique<Column>(AttrType::INTS, 4);
    for (int i = 0; i < 1023; i++) {
      int key = i % 8;
      column1->append_one((char *)&key);
      column2->append_one((char *)&i);
    }
    group_chunk.add_column(std::move(column1), 0);
    aggr_chunk.add_column(std::move(column2), 1);

    AggregateExpr             aggregate_expr(AggregateExpr::Type::SUM, nullptr);
    std::vector<Expression *> aggregate_exprs;
    aggregate_exprs.push_back(&aggregate_expr);
    auto standard_hash_table = std::make_unique<StandardAggregateHashTable>(aggregate_exprs);
    RC   rc                  = standard_hash_table->add_chunk(group_chunk, aggr_chunk);
    ASSERT_EQ(rc, RC::SUCCESS);
    Chunk output_chunk;
    output_chunk.add_column(
        make_unique<Column>(group_chunk.column(0).attr_type(), group_chunk.column(0).attr_len()), 0);
    output_chunk.add_column(make_unique<Column>(aggr_chunk.column(0).attr_type(), aggr_chunk.column(0).attr_len()), 1);
    StandardAggregateHashTable::Scanner scanner(standard_hash_table.get());
    scanner.open_scan();
    rc = scanner.next(output_chunk);
    ASSERT_EQ(rc, RC::SUCCESS);
    ASSERT_EQ(output_chunk.rows(), 8);
    for (int i = 0; i < 8; i++) {
      std::cout << "column1: " << output_chunk.get_value(1, i).get_string() << " "
                << "sum(column2)" << output_chunk.get_value(0, i).get_string() << std::endl;
    }
  }
  // mutiple group by columns, mutiple aggregate columns
  {
    Chunk                   group_chunk;
    Chunk                   aggr_chunk;
    std::unique_ptr<Column> group1 = std::make_unique<Column>(AttrType::CHARS, 4);
    std::unique_ptr<Column> group2 = std::make_unique<Column>(AttrType::INTS, 4);

    std::unique_ptr<Column> aggr1 = std::make_unique<Column>(AttrType::FLOATS, 4);
    std::unique_ptr<Column> aggr2 = std::make_unique<Column>(AttrType::INTS, 4);
    for (int i = 0; i < 1023; i++) {
      float i_float  = i + 0.5;
      int   i_group2 = i % 8;

      group1->append_one((char *)std::to_string(i % 8).c_str());
      group2->append_one((char *)&i_group2);
      aggr1->append_one((char *)&i_float);
      aggr2->append_one((char *)&i);
    }
    group_chunk.add_column(std::move(group1), 0);
    group_chunk.add_column(std::move(group2), 1);
    aggr_chunk.add_column(std::move(aggr1), 0);
    aggr_chunk.add_column(std::move(aggr2), 1);

    AggregateExpr             aggregate_expr(AggregateExpr::Type::SUM, nullptr);
    std::vector<Expression *> aggregate_exprs;
    aggregate_exprs.push_back(&aggregate_expr);
    aggregate_exprs.push_back(&aggregate_expr);
    auto standard_hash_table = std::make_unique<StandardAggregateHashTable>(aggregate_exprs);
    RC   rc                  = standard_hash_table->add_chunk(group_chunk, aggr_chunk);
    ASSERT_EQ(rc, RC::SUCCESS);
    Chunk output_chunk;
    output_chunk.add_column(
        make_unique<Column>(group_chunk.column(0).attr_type(), group_chunk.column(0).attr_len()), 0);
    output_chunk.add_column(
        make_unique<Column>(group_chunk.column(1).attr_type(), group_chunk.column(1).attr_len()), 1);
    output_chunk.add_column(make_unique<Column>(aggr_chunk.column(0).attr_type(), aggr_chunk.column(0).attr_len()), 1);
    output_chunk.add_column(make_unique<Column>(aggr_chunk.column(1).attr_type(), aggr_chunk.column(1).attr_len()), 2);
    StandardAggregateHashTable::Scanner scanner(standard_hash_table.get());
    scanner.open_scan();
    rc = scanner.next(output_chunk);
    ASSERT_EQ(rc, RC::SUCCESS);
    ASSERT_EQ(output_chunk.rows(), 8);
    for (int i = 0; i < 8; i++) {
      std::cout << "group1: " << output_chunk.get_value(0, i).get_string() << " "
                << "group2: " << output_chunk.get_value(1, i).get_string() << " "
                << "sum(aggr1): " << output_chunk.get_value(2, i).get_string() << " "
                << "sum(aggr2): " << output_chunk.get_value(3, i).get_string() << std::endl;
    }
  }
}

#ifdef USE_SIMD
TEST(AggregateHashTableTest, DISABLED_linear_probing_hash_table)
{
  // simple case
  {
    Chunk                   group_chunk;
    Chunk                   aggr_chunk;
    std::unique_ptr<Column> column1 = std::make_unique<Column>(AttrType::INTS, 4);
    std::unique_ptr<Column> column2 = std::make_unique<Column>(AttrType::INTS, 4);
    for (int i = 0; i < 1023; i++) {
      int key = i % 8;
      column1->append_one((char *)&key);
      column2->append_one((char *)&i);
    }
    group_chunk.add_column(std::move(column1), 0);
    aggr_chunk.add_column(std::move(column2), 1);

    auto linear_probing_hash_table = std::make_unique<LinearProbingAggregateHashTable<int>>(AggregateExpr::Type::SUM);
    RC   rc                        = linear_probing_hash_table->add_chunk(group_chunk, aggr_chunk);
    ASSERT_EQ(rc, RC::SUCCESS);
    Chunk output_chunk;
    output_chunk.add_column(
        make_unique<Column>(group_chunk.column(0).attr_type(), group_chunk.column(0).attr_len()), 0);
    output_chunk.add_column(make_unique<Column>(aggr_chunk.column(0).attr_type(), aggr_chunk.column(0).attr_len()), 1);
    LinearProbingAggregateHashTable<int>::Scanner scanner(linear_probing_hash_table.get());
    scanner.open_scan();
    rc = scanner.next(output_chunk);
    ASSERT_EQ(rc, RC::SUCCESS);
    ASSERT_EQ(output_chunk.rows(), 8);
    for (int i = 0; i < 8; i++) {
      std::cout << "column1: " << output_chunk.get_value(1, i).get_string() << " "
                << "sum(column2)" << output_chunk.get_value(0, i).get_string() << std::endl;
    }
  }

  // hash conflict
  {
    Chunk                   group_chunk;
    Chunk                   aggr_chunk;
    std::unique_ptr<Column> column1 = std::make_unique<Column>(AttrType::INTS, 4);
    std::unique_ptr<Column> column2 = std::make_unique<Column>(AttrType::INTS, 4);
    for (int i = 0; i < 1002; i++) {
      int key, value = 1;
      if (i % 2 == 0) {
        key = 1;
      } else {
        key = 257;
      }

      column1->append_one((char *)&key);
      column2->append_one((char *)&value);
    }
    group_chunk.add_column(std::move(column1), 0);
    aggr_chunk.add_column(std::move(column2), 1);

    auto linear_probing_hash_table =
        std::make_unique<LinearProbingAggregateHashTable<int>>(AggregateExpr::Type::SUM, 256);
    RC rc = linear_probing_hash_table->add_chunk(group_chunk, aggr_chunk);
    ASSERT_EQ(rc, RC::SUCCESS);
    Chunk output_chunk;
    output_chunk.add_column(
        make_unique<Column>(group_chunk.column(0).attr_type(), group_chunk.column(0).attr_len()), 0);
    output_chunk.add_column(make_unique<Column>(aggr_chunk.column(0).attr_type(), aggr_chunk.column(0).attr_len()), 1);
    LinearProbingAggregateHashTable<int>::Scanner scanner(linear_probing_hash_table.get());
    scanner.open_scan();
    rc = scanner.next(output_chunk);
    ASSERT_EQ(rc, RC::SUCCESS);
    ASSERT_EQ(output_chunk.rows(), 2);
    ASSERT_STREQ(output_chunk.get_value(1, 0).get_string().c_str(), "501");
    ASSERT_STREQ(output_chunk.get_value(1, 1).get_string().c_str(), "501");
  }
}
#endif

int main(int argc, char **argv)
{

  // 分析gtest程序的命令行参数
  testing::InitGoogleTest(&argc, argv);

  // 调用RUN_ALL_TESTS()运行所有测试用例
  // main函数返回RUN_ALL_TESTS()的运行结果
  return RUN_ALL_TESTS();
}
