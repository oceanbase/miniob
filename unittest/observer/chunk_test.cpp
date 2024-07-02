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

#include "storage/common/chunk.h"
#include "gtest/gtest.h"

using namespace std;

TEST(ChunkTest, chunk_test)
{
  // empty
  {
    Chunk chunk;
    ASSERT_EQ(chunk.capacity(), 0);
    ASSERT_EQ(chunk.column_num(), 0);
    ASSERT_EQ(chunk.rows(), 0);
  }
  // normal
  {
    int   row_num = 8;
    Chunk chunk;
    chunk.add_column(std::make_unique<Column>(AttrType::INTS, sizeof(int), row_num), 0);
    chunk.add_column(std::make_unique<Column>(AttrType::FLOATS, sizeof(float), row_num), 1);
    ASSERT_EQ(chunk.capacity(), row_num);
    ASSERT_EQ(chunk.column_num(), 2);
    ASSERT_EQ(chunk.rows(), 0);
    for (int i = 0; i < row_num; i++) {
      int value1 = i;
      chunk.column(0).append_one((char *)&value1);
    }
    for (int i = 0; i < row_num; i++) {
      float value2 = i + 0.5f;
      chunk.column(1).append_one((char *)&value2);
    }
    ASSERT_EQ(chunk.rows(), row_num);

    Chunk chunk2;
    chunk2.reference(chunk);
    ASSERT_EQ(chunk2.capacity(), row_num);
    ASSERT_EQ(chunk2.column_num(), 2);
    ASSERT_EQ(chunk2.rows(), row_num);
    for (int i = 0; i < row_num; i++) {
      int value1 = i;
      ASSERT_EQ(chunk2.get_value(0, i).get_int(), value1);
    }

    for (int i = 0; i < row_num; i++) {
      float value2 = i + 0.5f;
      ASSERT_EQ(chunk2.get_value(1, i).get_float(), value2);
    }
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
