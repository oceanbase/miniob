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
// Created by Wangyunlai on 2023/06/16.
//

#include "gtest/gtest.h"

#include "net/ring_buffer.h"

TEST(ring_buffer, test_init)
{
  const int buf_size = 10;
  RingBuffer buffer(buf_size);
  EXPECT_EQ(buffer.capacity(), buf_size);
  EXPECT_EQ(buffer.remain(), buf_size);
  EXPECT_EQ(buffer.size(), 0);
  EXPECT_EQ(buffer.remain(), 10);
}

TEST(ring_buffer, test_write)
{
  const int buf_size = 25;
  RingBuffer buffer(buf_size);

  const char *data = "0123456789";
  int32_t size = strlen(data);
  int32_t write_size = 0;
  EXPECT_EQ(buffer.write(data, size, write_size), RC::SUCCESS);
  EXPECT_EQ(write_size, size);
  EXPECT_EQ(buffer.size(), size);
  EXPECT_EQ(buffer.remain(), buf_size - size);

  EXPECT_EQ(buffer.write(data, size, write_size), RC::SUCCESS);
  EXPECT_EQ(write_size, size);
  EXPECT_EQ(buffer.size(), size * 2);
  EXPECT_EQ(buffer.remain(), buf_size - size * 2);

  EXPECT_EQ(buffer.write(data, size, write_size), RC::SUCCESS);
  EXPECT_EQ(write_size, 5);
  EXPECT_EQ(buffer.size(), buffer.capacity());
  EXPECT_EQ(buffer.remain(), 0);
}

TEST(ring_buffer, test_read)
{
  const int buf_size = 100;
  RingBuffer buffer(buf_size);

  const char *data = "0123456789";
  int32_t size = strlen(data);
  int32_t write_size = 0;
  EXPECT_EQ(buffer.write(data, size, write_size), RC::SUCCESS);
  EXPECT_EQ(write_size, size);
  EXPECT_EQ(buffer.size(), size);
  EXPECT_EQ(buffer.remain(), buf_size - size);

  char read_buf[buf_size];
  int32_t read_size = 0;
  const int32_t test_read_size = 5;
  EXPECT_EQ(buffer.read(read_buf, test_read_size, read_size), RC::SUCCESS);
  EXPECT_EQ(read_size, test_read_size);
  EXPECT_EQ(buffer.size(), test_read_size);
  EXPECT_EQ(buffer.remain(), buf_size - test_read_size);

  EXPECT_EQ(buffer.read(read_buf, test_read_size, read_size), RC::SUCCESS);
  EXPECT_EQ(read_size, test_read_size);
  EXPECT_EQ(buffer.size(), 0);
  EXPECT_EQ(buffer.remain(), buf_size);

  EXPECT_EQ(buffer.read(read_buf, test_read_size, read_size), RC::SUCCESS);
  EXPECT_EQ(read_size, 0);
  EXPECT_EQ(buffer.size(), 0);
  EXPECT_EQ(buffer.remain(), buf_size);
}

TEST(ring_buffer, test_buffer)
{
  const int buf_size = 15;
  RingBuffer buffer(buf_size);

  const char *data = "0123456789";
  int32_t size = strlen(data);
  int32_t write_size = 0;
  EXPECT_EQ(buffer.write(data, size, write_size), RC::SUCCESS);
  EXPECT_EQ(write_size, size);
  EXPECT_EQ(buffer.size(), size);
  EXPECT_EQ(buffer.remain(), buf_size - size);

  const char *tmp_buffer = nullptr;
  int32_t buffer_size = 0;
  EXPECT_EQ(buffer.buffer(tmp_buffer, buffer_size), RC::SUCCESS);
  EXPECT_EQ(buffer_size, size);
  EXPECT_EQ(buffer.forward(buffer_size), RC::SUCCESS);

  EXPECT_EQ(buffer.buffer(tmp_buffer, buffer_size), RC::SUCCESS);
  EXPECT_EQ(buffer_size, 0);

  EXPECT_EQ(buffer.write(data, size, write_size), RC::SUCCESS);
  EXPECT_EQ(write_size, size);

  EXPECT_EQ(buffer.buffer(tmp_buffer, buffer_size), RC::SUCCESS);
  EXPECT_LT(buffer_size, size);
  EXPECT_EQ(buffer.forward(buffer_size), RC::SUCCESS);

  EXPECT_EQ(buffer.buffer(tmp_buffer, buffer_size), RC::SUCCESS);
  EXPECT_LT(buffer_size, size);
  EXPECT_EQ(buffer.forward(buffer_size), RC::SUCCESS);
}

int main(int argc, char **argv)
{
  // 分析gtest程序的命令行参数
  testing::InitGoogleTest(&argc, argv);

  // 调用RUN_ALL_TESTS()运行所有测试用例
  // main函数返回RUN_ALL_TESTS()的运行结果
  return RUN_ALL_TESTS();
}