/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by wangyunlai.wyl on 2021
//

#include "storage/default/disk_buffer_pool.h"
#include "gtest/gtest.h"

TEST(test_bp_manager, test_bp_manager_simple_lru) {
  BPManager bp_manager(2);

  Frame * frame1 = bp_manager.alloc();
  ASSERT_NE(frame1, nullptr);

  frame1->file_desc = 0;
  frame1->page.page_num = 1;

  ASSERT_EQ(frame1, bp_manager.get(0, 1));

  Frame *frame2 = bp_manager.alloc();
  ASSERT_NE(frame2, nullptr);
  frame2->file_desc = 0;
  frame2->page.page_num = 2;

  ASSERT_EQ(frame1, bp_manager.get(0, 1));

  Frame *frame3 = bp_manager.alloc();
  ASSERT_NE(frame3, nullptr);
  frame3->file_desc = 0;
  frame3->page.page_num = 3;

  frame2 = bp_manager.get(0, 2);
  ASSERT_EQ(frame2, nullptr);

  Frame *frame4 = bp_manager.alloc();
  frame4->file_desc = 0;
  frame4->page.page_num = 4;

  frame1 = bp_manager.get(0, 1);
  ASSERT_EQ(frame1, nullptr);

  frame3 = bp_manager.get(0, 3);
  ASSERT_NE(frame3, nullptr);

  frame4 = bp_manager.get(0, 4);
  ASSERT_NE(frame4, nullptr);
}

int main(int argc, char **argv) {


  // 分析gtest程序的命令行参数
  testing::InitGoogleTest(&argc, argv);

  // 调用RUN_ALL_TESTS()运行所有测试用例
  // main函数返回RUN_ALL_TESTS()的运行结果
  return RUN_ALL_TESTS();
}