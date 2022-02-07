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
// Created by longda on 2022
//

#include <list>
#include <iostream>

#include "storage/common/bplus_tree.h"
#include "storage/default/disk_buffer_pool.h"
#include "rc.h"
#include "common/log/log.h"
#include "sql/parser/parse_defs.h"
#include "gtest/gtest.h"

using namespace common;

#define TIMES 3
// order must small real order
// if page is 8k, it is 400
#define ORDER 4
#define INSERT_NUM (TIMES * ORDER * ORDER * ORDER * ORDER)
#define POOL_NUM 2

BplusTreeHandler *handler = nullptr;
const char *index_name = "test.btree";
int insert_num = INSERT_NUM;
const int page_size = 1024;
RID rid, check_rid;
int k = 0;

void test_insert()
{
  for (int i = 0; i < insert_num; i++) {

    rid.page_num = i / page_size;
    rid.slot_num = i % page_size;

    if (i % TIMES == 1) {
      if (insert_num > page_size) {
        if (k++ % 100 == 0) {
          LOG_INFO("Begin to insert the page's num %s", rid.to_string().c_str());
        }
      } else {
        LOG_INFO("Insert %d", i);
      }
      RC rc = handler->insert_entry((const char *)&i, &rid);
      ASSERT_EQ(RC::SUCCESS, rc);
      ASSERT_EQ(true, handler->validate_tree());
    }
  }
  handler->print_tree();

  for (int i = 0; i < insert_num; i++) {

    rid.page_num = i / page_size;
    rid.slot_num = i % page_size;

    if (i % TIMES == 2) {
      if (insert_num > page_size) {
        if (k++ % 100 == 0) {
          LOG_INFO("Begin to insert the page's num %s", rid.to_string().c_str());
        }
      } else {
        LOG_INFO("Insert %d", i);
      }
      RC rc = handler->insert_entry((const char *)&i, &rid);
      ASSERT_EQ(RC::SUCCESS, rc);
      ASSERT_EQ(true, handler->validate_tree());
    }
  }

  handler->print_tree();
  for (int i = 0; i < insert_num; i++) {

    rid.page_num = i / page_size;
    rid.slot_num = i % page_size;

    if (i % TIMES == 0) {
      if (insert_num > page_size) {
        if (k++ % 100 == 0) {
          LOG_INFO("Begin to insert the page's num %s", rid.to_string().c_str());
        }
      } else {
        LOG_INFO("Insert %d", i);
      }
      RC rc = handler->insert_entry((const char *)&i, &rid);
      ASSERT_EQ(RC::SUCCESS, rc);
      ASSERT_EQ(true, handler->validate_tree());
    }
  }

  LOG_INFO("@@@@ finish first step insert");
  handler->print_tree();
  handler->print_leafs();

  int start = insert_num / TIMES > page_size ? page_size : insert_num / TIMES;
  int end = insert_num / TIMES > page_size ? (2 * page_size) : (2 * insert_num / TIMES);
  for (int i = start; i < end; i++) {
    rid.page_num = i / page_size;
    rid.slot_num = i % page_size;

    if (insert_num > page_size) {
      if (k++ % 100 == 0) {
        LOG_INFO("Begin to check duplicated insert the page's num %s", rid.to_string().c_str());
      }
    } else {
      LOG_INFO("Check duplicate insert %d", i);
    }
    RC rc = handler->insert_entry((const char *)&i, &rid);
    int t = i % TIMES;
    if (t == 0 || t == 1 || t == 2) {
      ASSERT_EQ(RC::RECORD_DUPLICATE_KEY, rc);
    } else {
      ASSERT_EQ(RC::SUCCESS, rc);
      ASSERT_EQ(true, handler->validate_tree());
    }
  }
}

void test_get()
{
  std::list<RID> rids;
  for (int i = 0; i < insert_num; i++) {
    rid.page_num = i / page_size;
    rid.slot_num = i % page_size;
    if (insert_num > page_size) {
      if (k++ % 100 == 0) {
        LOG_INFO("Begin to get every entry of index,  rid: %s", rid.to_string().c_str());
      }
    } else {
      LOG_INFO("Begin to get every entry of index,  rid: %s", rid.to_string().c_str());
    }

    rids.clear();
    RC rc = handler->get_entry((const char *)&i, rids);

    ASSERT_EQ(RC::SUCCESS, rc);
    ASSERT_EQ(1, rids.size());
    check_rid = rids.front();
    ASSERT_EQ(rid.page_num, check_rid.page_num);
    ASSERT_EQ(rid.slot_num, check_rid.slot_num);
  }
}

void test_delete()
{
  std::list<RID> rids;

  for (int i = 0; i < insert_num / 2; i++) {
    rid.page_num = i / page_size;
    rid.slot_num = i % page_size;

    int t = i % TIMES;
    if (t == 0 || t == 1) {
      if (insert_num > page_size) {
        if (k++ % 100 == 0) {
          LOG_INFO("Begin to delete entry of index,  rid: %s", rid.to_string().c_str());
        }
      } else {
        LOG_INFO("Begin to delete entry of index,  rid: %s", rid.to_string().c_str());
      }

      RC rc = handler->delete_entry((const char *)&i, &rid);

      ASSERT_EQ(true, handler->validate_tree());
      ASSERT_EQ(RC::SUCCESS, rc);
    }
  }

  handler->print_tree();

  for (int i = insert_num - 1; i >= insert_num / 2; i--) {
    rid.page_num = i / page_size;
    rid.slot_num = i % page_size;

    int t = i % TIMES;
    if (t == 0 || t == 1) {
      if (insert_num > page_size) {
        if (k++ % 100 == 0) {
          LOG_INFO("Begin to delete entry of index,  rid: %s", rid.to_string().c_str());
        }
      } else {
        LOG_INFO("Begin to delete entry of index,  rid: %s", rid.to_string().c_str());
      }
      RC rc = handler->delete_entry((const char *)&i, &rid);

      ASSERT_EQ(true, handler->validate_tree());
      ASSERT_EQ(RC::SUCCESS, rc);
    }
  }
  handler->print_tree();

  for (int i = 0; i < insert_num; i++) {
    rid.page_num = i / page_size;
    rid.slot_num = i % page_size;
    if (insert_num > page_size) {
      if (k++ % 100 == 0) {
        LOG_INFO("Begin to get entry of index,  rid: %s", rid.to_string().c_str());
      }
    } else {
      LOG_INFO("Begin to get entry of index,  rid: %s", rid.to_string().c_str());
    }
    rids.clear();
    RC rc = handler->get_entry((const char *)&i, rids);
    ASSERT_EQ(RC::SUCCESS, rc);
    int t = i % TIMES;
    if (t == 0 || t == 1) {
      ASSERT_EQ(0, rids.size());
    } else {
      ASSERT_EQ(1, rids.size());
      check_rid = rids.front();
      ASSERT_EQ(rid.page_num, check_rid.page_num);
      ASSERT_EQ(rid.slot_num, check_rid.slot_num);
      ASSERT_EQ(true, handler->validate_tree());
    }
  }

  handler->print_tree();
  for (int i = 0; i < insert_num / 2; i++) {
    rid.page_num = i / page_size;
    rid.slot_num = i % page_size;

    int t = i % TIMES;
    if (t == 2) {
      if (insert_num > page_size) {
        if (k++ % 100 == 0) {
          LOG_INFO("Begin to delete entry of index,  rid: %s", rid.to_string().c_str());
        }
      } else {
        LOG_INFO("Begin to delete entry of index,  rid: %s", rid.to_string().c_str());
      }
      RC rc = handler->delete_entry((const char *)&i, &rid);

      ASSERT_EQ(true, handler->validate_tree());
      ASSERT_EQ(RC::SUCCESS, rc);
    }
  }

  handler->print_tree();

  for (int i = insert_num - 1; i >= insert_num / 2; i--) {
    rid.page_num = i / page_size;
    rid.slot_num = i % page_size;

    int t = i % TIMES;
    if (t == 2) {
      if (insert_num > page_size) {
        if (k++ % 100 == 0) {
          LOG_INFO("Begin to delete entry of index,  rid: %s", rid.to_string().c_str());
        }
      } else {
        LOG_INFO("Begin to delete entry of index,  rid: %s", rid.to_string().c_str());
      }
      RC rc = handler->delete_entry((const char *)&i, &rid);

      ASSERT_EQ(true, handler->validate_tree());
      ASSERT_EQ(RC::SUCCESS, rc);
    }
  }

  handler->print_tree();

  for (int i = 0; i < insert_num; i++) {
    rid.page_num = i / page_size;
    rid.slot_num = i % page_size;
    if (insert_num > page_size) {
      if (k++ % 100 == 0) {
        LOG_INFO("Begin to insert entry of index,  rid: %s", rid.to_string().c_str());
      }
    } else {
      LOG_INFO("Begin to insert entry of index,  rid: %s", rid.to_string().c_str());
    }
    RC rc = handler->insert_entry((const char *)&i, &rid);
    int t = i % TIMES;
    if (t == 0 || t == 1 || t == 2) {
      ASSERT_EQ(RC::SUCCESS, rc);
      ASSERT_EQ(true, handler->validate_tree());
    } else {
      ASSERT_EQ(RC::RECORD_DUPLICATE_KEY, rc);
    }
  }
  handler->print_tree();
}

TEST(test_bplus_tree, test_bplus_tree_insert)
{

  LoggerFactory::init_default("test.log");
  // set the disk buffer pool's number to make it is easy to test
  DiskBufferPool::set_pool_num(POOL_NUM);

  ::remove(index_name);
  handler = new BplusTreeHandler();
  handler->create(index_name, INTS, sizeof(int));

  BplusTreeTester bplus_tree_tester(*handler);
  bplus_tree_tester.set_order(ORDER);

  test_insert();

  test_get();

  test_delete();

  handler->close();
  delete handler;
  handler = nullptr;
}

int main(int argc, char **argv)
{

  // 分析gtest程序的命令行参数
  testing::InitGoogleTest(&argc, argv);

  // 调用RUN_ALL_TESTS()运行所有测试用例
  // main函数返回RUN_ALL_TESTS()的运行结果

  int rc = RUN_ALL_TESTS();

  return rc;
}