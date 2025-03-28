/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "gtest/gtest.h"
#include <cstdio>
#include <filesystem>

#include "oblsm/util/ob_comparator.h"
#include "oblsm/util/ob_file_reader.h"
#include "oblsm/util/ob_file_writer.h"
#include "common/lang/filesystem.h"

using namespace oceanbase;

TEST(util_test, DISABLED_comparator_test_basic)
{
  ObDefaultComparator comparator;
  EXPECT_TRUE(comparator.compare("key99", "key999") < 0);
  EXPECT_TRUE(comparator.compare("key100", "key10") > 0);
  EXPECT_TRUE(comparator.compare("key111", "key111") == 0);
}

TEST(util_test, DISABLED_create_file) {
  remove("tmpfile");
  auto w = ObFileWriter::create_file_writer("tmpfile", false);
  w->open_file();
  EXPECT_TRUE(filesystem::exists("tmpfile"));
  remove("tmpfile");
}

TEST(util_test, DISABLED_rw_file) {
  remove("tmpfile");
  auto w = ObFileWriter::create_file_writer("tmpfile", false);
  w->open_file();
  string s = "hello world";
  w->write(s);
  w->flush();
  auto r = ObFileReader::create_file_reader("tmpfile");
  auto read_res = r->read_pos(0, s.size());
  EXPECT_EQ(read_res, s);
  EXPECT_TRUE(filesystem::exists("tmpfile"));
  remove("tmpfile");
}


int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}