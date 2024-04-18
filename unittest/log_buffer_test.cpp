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

#define private public
#define protected public
#include "storage/clog/log_buffer.h"
#include "storage/clog/log_file.h"

using namespace std;
using namespace common;

TEST(LogEntryBuffer, test_append)
{
  // test init LogEntryBuffer and append
  LSN            start_lsn = 1000;
  LSN            end_lsn   = 2000 - 1;
  LogEntryBuffer buffer;
  ASSERT_EQ(RC::SUCCESS, buffer.init(start_lsn));

  vector<char> data(10);

  LSN lsn = 0;
  ASSERT_EQ(RC::SUCCESS, buffer.append(lsn, LogModule::Id::BUFFER_POOL, std::move(data)));
  ASSERT_EQ(lsn, start_lsn + 1);
  ASSERT_EQ(buffer.current_lsn(), lsn);

  ASSERT_GT(buffer.bytes(), 0);
  ASSERT_GT(buffer.entry_number(), 0);

  LogFileWriter writer;
  ASSERT_EQ(RC::SUCCESS, writer.open("test_log_entry_buffer.log", end_lsn));
  int count = 0;
  ASSERT_EQ(RC::SUCCESS, buffer.flush(writer, count));
  ASSERT_EQ(count, 1);

  for (LSN i = lsn + 1; i <= end_lsn; i++) {
    vector<char> data(10);
    ASSERT_EQ(RC::SUCCESS, buffer.append(lsn, LogModule::Id::BUFFER_POOL, std::move(data)));
    ASSERT_EQ(lsn, i);
    ASSERT_EQ(buffer.current_lsn(), lsn);
  }

  while (buffer.entry_number() > 0) {
    ASSERT_EQ(RC::SUCCESS, buffer.flush(writer, count));
  }
  ASSERT_EQ(buffer.flushed_lsn(), end_lsn);

  ASSERT_EQ(RC::SUCCESS, buffer.append(lsn, LogModule::Id::BUFFER_POOL, vector<char>(10)));

  ASSERT_NE(RC::SUCCESS, buffer.flush(writer, count));

  writer.close();
  filesystem::remove("test_log_entry_buffer.log");
}

int main(int argc, char **argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
