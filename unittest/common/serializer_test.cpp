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
// Created by wangyunlai.wyl on 2024/02/21
//

#include "gtest/gtest.h"
#include "common/lang/serializer.h"
#include "common/lang/filesystem.h"
#include "common/log/log.h"

using namespace common;

TEST(Serializer, serializer)
{
  char str[] = "12345";

  Serializer serializer;
  int32_t    a0 = 1;
  int64_t    b0 = 2L + INT32_MAX;
  int32_t    c0 = 3;
  int64_t    d0 = 4L + INT32_MAX;
  serializer.write_int32(a0);
  serializer.write_int64(b0);
  serializer.write_int32(c0);
  serializer.write_int64(d0);
  serializer.write(span<const char>(str, sizeof(str)));

  Deserializer deserializer(serializer.data());
  int32_t      a;
  int64_t      b;
  int32_t      c;
  int64_t      d;
  deserializer.read_int32(a);
  deserializer.read_int64(b);
  deserializer.read_int32(c);
  deserializer.read_int64(d);

  char str2[sizeof(str)];
  deserializer.read(span<char>(str2, sizeof(str2)));

  ASSERT_EQ(a, a0);
  ASSERT_EQ(b, b0);
  ASSERT_EQ(c, c0);
  ASSERT_EQ(d, d0);
  ASSERT_EQ(0, memcmp(str, str2, sizeof(str)));

  char str3[sizeof(str)];
  int  ret = deserializer.read(span<char>(str3, sizeof(str3)));
  ASSERT_NE(ret, 0);
}

int main(int argc, char **argv)
{
  testing::InitGoogleTest(&argc, argv);
  filesystem::path log_filename = filesystem::path(argv[0]).filename();
  LoggerFactory::init_default(log_filename.string() + ".log", LOG_LEVEL_TRACE);
  return RUN_ALL_TESTS();
}