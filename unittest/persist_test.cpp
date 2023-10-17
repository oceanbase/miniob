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
// Created by qiling on 2022
//

#include <string.h>
#include "gtest/gtest.h"
#include "storage/persist/persist.h"

const int MAX_LEN = 50;

TEST(test_persist, test_persist_file_io)
{
  std::string file_name_1 = "test_persist_file_io-file_name_1";
  std::string file_name_2 = "test_persist_file_io-file_name_2";
  PersistHandler persist_handler;
  PersistHandler persist_handler_2;
  RC rc;

  // prepare
  remove(file_name_1.c_str());
  remove(file_name_2.c_str());

  // create
  rc = persist_handler.create_file(nullptr);
  ASSERT_EQ(rc, RC::FILE_NAME);

  rc = persist_handler.create_file(file_name_1.c_str());
  ASSERT_EQ(rc, RC::SUCCESS);
  ASSERT_NE(access(file_name_1.c_str(), F_OK), -1);

  rc = persist_handler.create_file(file_name_2.c_str());
  ASSERT_EQ(rc, RC::FILE_BOUND);

  rc = persist_handler_2.create_file("");
  ASSERT_EQ(rc, RC::FILE_CREATE);

  // open
  rc = persist_handler_2.open_file();
  ASSERT_EQ(rc, RC::FILE_NAME);

  rc = persist_handler_2.open_file("//**");
  ASSERT_EQ(rc, RC::FILE_OPEN);

  rc = persist_handler.open_file(file_name_2.c_str());
  ASSERT_EQ(rc, RC::FILE_BOUND);

  rc = persist_handler.open_file();
  ASSERT_EQ(rc, RC::SUCCESS);

  // write
  std::string str_1 = "this is a string 001. ";
  std::string str_2 = "this is a string 002002. ";
  std::string str_3 = "THIS IS A STRING 003. ";
  int64_t write_size = 0;

  rc = persist_handler.write_file(str_1.size(), str_1.c_str(), &write_size);
  ASSERT_EQ(rc, RC::SUCCESS);
  ASSERT_EQ(write_size, str_1.size());

  rc = persist_handler.append(str_2.size(), str_2.c_str(), &write_size);
  ASSERT_EQ(rc, RC::SUCCESS);
  ASSERT_EQ(write_size, str_2.size());

  rc = persist_handler.write_at(str_1.size() + str_2.size(), str_3.size(), str_3.c_str(), &write_size);
  ASSERT_EQ(rc, RC::SUCCESS);
  ASSERT_EQ(write_size, str_3.size());

  rc = persist_handler.close_file();
  ASSERT_EQ(rc, RC::SUCCESS);

  rc = persist_handler.append(str_3.size(), str_3.c_str(), &write_size);
  ASSERT_EQ(rc, RC::FILE_NOT_OPENED);

  rc = persist_handler.open_file();
  ASSERT_EQ(rc, RC::SUCCESS);

  rc = persist_handler_2.write_at(0, str_3.size(), str_3.c_str(), &write_size);
  ASSERT_EQ(rc, RC::FILE_NOT_EXIST);

  // read & seek
  int64_t read_size = 0;
  char buf[MAX_LEN] = {0};

  rc = persist_handler.read_at(str_1.size(), str_2.size(), buf, &read_size);
  ASSERT_EQ(rc, RC::SUCCESS);
  ASSERT_EQ(read_size, str_2.size());
  ASSERT_EQ(strnlen(buf, MAX_LEN), str_2.size());
  ASSERT_EQ(strncmp(buf, str_2.c_str(), MAX_LEN), 0);

  rc = persist_handler.seek(0);
  ASSERT_EQ(rc, RC::SUCCESS);

  rc = persist_handler.write_file(str_3.size(), str_3.c_str(), &write_size);
  ASSERT_EQ(rc, RC::SUCCESS);
  ASSERT_EQ(write_size, str_3.size());

  memset(buf, 0, MAX_LEN);

  rc = persist_handler.seek(0);
  ASSERT_EQ(rc, RC::SUCCESS);

  rc = persist_handler.read_file(str_3.size(), buf, &read_size);
  ASSERT_EQ(rc, RC::SUCCESS);
  ASSERT_EQ(read_size, str_3.size());
  ASSERT_EQ(strnlen(buf, MAX_LEN), str_3.size());
  ASSERT_EQ(strncmp(buf, str_3.c_str(), MAX_LEN), 0);

  rc = persist_handler_2.read_at(0, str_3.size(), buf, &read_size);
  ASSERT_EQ(rc, RC::FILE_NOT_EXIST);

  // close
  rc = persist_handler.close_file();
  ASSERT_EQ(rc, RC::SUCCESS);

  rc = persist_handler_2.close_file();
  ASSERT_EQ(rc, RC::SUCCESS);

  // remove
  rc = persist_handler.remove_file();
  ASSERT_EQ(rc, RC::SUCCESS);
  ASSERT_EQ(access(file_name_1.c_str(), F_OK), -1);
}

int main(int argc, char **argv)
{

  // 分析gtest程序的命令行参数
  testing::InitGoogleTest(&argc, argv);

  // 调用RUN_ALL_TESTS()运行所有测试用例
  // main函数返回RUN_ALL_TESTS()的运行结果
  return RUN_ALL_TESTS();
}
