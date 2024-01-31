/* Copyright (c) 2021-2022 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by wangyunlai on 2024/01/31
//

#include "gtest/gtest.h"
#include "storage/clog/log_entry.h"

using namespace std;

TEST(LogHeader, to_string)
{
  LogHeader header;
  header.lsn = 1;
  header.size = 2;
  header.module_id = 3;

  header.to_string();
}

TEST(LogEntry, init)
{
  LogEntry entry;
  ASSERT_EQ(RC::SUCCESS, entry.init(1, LogModule::Id::BPLUS_TREE, unique_ptr<char[]>(new char[10]), 10));
}

TEST(LogEntry, size)
{
  LogEntry entry;
  ASSERT_EQ(entry.init(1, LogModule::Id::BPLUS_TREE, unique_ptr<char[]>(new char[10]), 10), RC::SUCCESS);
  ASSERT_EQ(entry.size(), 10 + LogHeader::SIZE);

  unique_ptr<char[]> data(new char[10]);
  memcpy(data.get(), entry.data(), 10); // test that the data of entry can be copyed

  LogEntry entry2(std::move(entry));
  memcpy(data.get(), entry2.data(), 10);

  ASSERT_EQ(entry.init(1, LogModule::Id::BPLUS_TREE, unique_ptr<char[]>(new char[0]), 0), RC::SUCCESS);
  ASSERT_EQ(entry.size(), LogHeader::SIZE);

  // size is too large
  long long int size = LogEntry::max_payload_size() + 1;
  ASSERT_NE(entry.init(1, LogModule::Id::BPLUS_TREE, unique_ptr<char[]>(new char[size]), size), RC::SUCCESS);
}

int main(int argc, char **argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}