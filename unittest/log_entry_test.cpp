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

#include <span>

#include "gtest/gtest.h"
#include "storage/clog/log_entry.h"

using namespace std;

TEST(LogHeader, to_string)
{
  LogHeader header;
  header.lsn       = 1;
  header.size      = 2;
  header.module_id = 3;

  header.to_string();
}

TEST(LogEntry, init)
{
  LogEntry     entry;
  vector<char> data(10);
  ASSERT_EQ(RC::SUCCESS, entry.init(1, LogModule::Id::BPLUS_TREE, std::move(data)));
}

TEST(LogEntry, size)
{
  LogEntry     entry;
  vector<char> data(10);
  ASSERT_EQ(entry.init(1, LogModule::Id::BPLUS_TREE, std::move(data)), RC::SUCCESS);
  ASSERT_EQ(entry.total_size(), 10 + LogHeader::SIZE);

  vector<char> data1(10);
  memcpy(data1.data(), entry.data(), entry.payload_size());  // test that the data of entry can be copyed

  LogEntry entry2(std::move(entry));
  memcpy(data1.data(), entry2.data(), entry2.payload_size());

  vector<char> empty_data;
  ASSERT_EQ(entry.init(1, LogModule::Id::BPLUS_TREE, std::move(empty_data)), RC::SUCCESS);
  ASSERT_EQ(entry.total_size(), LogHeader::SIZE);

  // size is too large
  long long int size = LogEntry::max_payload_size() + 1;
  vector<char>  data2(size);
  ASSERT_NE(entry.init(1, LogModule::Id::BPLUS_TREE, std::move(data2)), RC::SUCCESS);
}

int main(int argc, char **argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}