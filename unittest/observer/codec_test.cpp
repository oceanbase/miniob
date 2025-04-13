/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "storage/common/codec.h"
#include "common/lang/string_view.h"

#include "gtest/gtest.h"

TEST(CodecTest, codec_test)
{
  bytes b;
  OrderedCode::append(b, uint64_t(1025));
  ASSERT_EQ( b[0], 0x02 );
  ASSERT_EQ( b[1], 0x04 );
  ASSERT_EQ( b[2], 0x01 );
  bytes c;
  OrderedCode::append(c, uint64_t(1));
  string_view bv((char *)b.data(), b.size());
  string_view cv((char *)c.data(), c.size());

  ASSERT_TRUE(bv > cv);

}

int main(int argc, char **argv)
{

  // 分析gtest程序的命令行参数
  testing::InitGoogleTest(&argc, argv);

  // 调用RUN_ALL_TESTS()运行所有测试用例
  // main函数返回RUN_ALL_TESTS()的运行结果
  return RUN_ALL_TESTS();
}
