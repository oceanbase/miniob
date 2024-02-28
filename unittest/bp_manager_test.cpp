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
// Created by wangyunlai.wyl on 2021
//

#include "storage/buffer/disk_buffer_pool.h"
#include "gtest/gtest.h"

void test_get(BPFrameManager &frame_manager)
{
  const int buffer_pool_id = 0;
  PageNum   page_num       = 1;
  Frame    *frame1         = frame_manager.alloc(buffer_pool_id, page_num);
  ASSERT_NE(frame1, nullptr);

  frame1->set_buffer_pool_id(buffer_pool_id);
  frame1->set_page_num(page_num);

  ASSERT_EQ(frame1, frame_manager.get(buffer_pool_id, 1));
  frame1->unpin();

  Frame *frame2 = frame_manager.alloc(buffer_pool_id, 2);
  ASSERT_NE(frame2, nullptr);
  frame2->set_buffer_pool_id(0);
  frame2->set_page_num(2);

  ASSERT_EQ(frame1, frame_manager.get(buffer_pool_id, 1));
  frame1->unpin();

  Frame *frame3 = frame_manager.alloc(buffer_pool_id, 3);
  ASSERT_NE(frame3, nullptr);
  frame3->set_buffer_pool_id(buffer_pool_id);
  frame3->set_page_num(3);

  frame2 = frame_manager.get(buffer_pool_id, 2);
  ASSERT_NE(frame2, nullptr);
  frame2->unpin();

  Frame *frame4 = frame_manager.alloc(buffer_pool_id, 4);
  frame4->set_buffer_pool_id(buffer_pool_id);
  frame4->set_page_num(4);

  frame_manager.free(buffer_pool_id, frame1->page_num(), frame1);
  frame1 = frame_manager.get(buffer_pool_id, 1);
  ASSERT_EQ(frame1, nullptr);

  ASSERT_EQ(frame3, frame_manager.get(buffer_pool_id, 3));
  frame3->unpin();

  ASSERT_EQ(frame4, frame_manager.get(buffer_pool_id, 4));
  frame4->unpin();

  frame_manager.free(buffer_pool_id, frame2->page_num(), frame2);
  frame_manager.free(buffer_pool_id, frame3->page_num(), frame3);
  frame_manager.free(buffer_pool_id, frame4->page_num(), frame4);

  ASSERT_EQ(nullptr, frame_manager.get(buffer_pool_id, 2));
  ASSERT_EQ(nullptr, frame_manager.get(buffer_pool_id, 3));
  ASSERT_EQ(nullptr, frame_manager.get(buffer_pool_id, 4));
}

void test_alloc(BPFrameManager &frame_manager)
{
  std::list<Frame *> used_list;

  const int buffer_pool_id = 0;
  size_t    size           = 0;
  for (; true; size++) {
    Frame *item = frame_manager.alloc(buffer_pool_id, size);
    if (item != nullptr) {
      item->set_buffer_pool_id(buffer_pool_id);
      item->set_page_num(size);
      used_list.push_back(item);
    } else {
      break;
    }
  }
  size++;

  ASSERT_EQ(used_list.size(), frame_manager.frame_num());

  for (size_t i = size; i < size * 2; i++) {
    Frame *item = frame_manager.alloc(buffer_pool_id, i);

    ASSERT_EQ(item, nullptr);
  }

  for (size_t i = size * 2; i < size * 10; i++) {
    if (i % 2 == 0) {  // from size * 2, that free one frame first
      Frame *item = used_list.front();
      used_list.pop_front();

      RC rc = frame_manager.free(buffer_pool_id, item->page_num(), item);
      ASSERT_EQ(rc, RC::SUCCESS);
    } else {
      Frame *item = frame_manager.alloc(buffer_pool_id, i);
      ASSERT_NE(item, nullptr);
      item->set_buffer_pool_id(buffer_pool_id);
      item->set_page_num(i);
      used_list.push_back(item);
    }

    ASSERT_EQ(used_list.size(), frame_manager.frame_num());
  }
}

TEST(test_frame_manager, test_frame_manager_simple_lru)
{
  BPFrameManager frame_manager("Test");
  frame_manager.init(2);

  test_get(frame_manager);

  test_alloc(frame_manager);

  frame_manager.cleanup();
}

int main(int argc, char **argv)
{

  // 分析gtest程序的命令行参数
  testing::InitGoogleTest(&argc, argv);

  // 调用RUN_ALL_TESTS()运行所有测试用例
  // main函数返回RUN_ALL_TESTS()的运行结果
  return RUN_ALL_TESTS();
}
