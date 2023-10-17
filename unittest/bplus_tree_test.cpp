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
// Created by longda on 2022
//

#include <list>
#include <iostream>

#include "storage/index/bplus_tree.h"
#include "storage/buffer/disk_buffer_pool.h"
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

BufferPoolManager bpm;
BplusTreeHandler *handler = nullptr;
const char *index_name = "test.btree";
int insert_num = INSERT_NUM;
const int page_size = 1024;
RID rid, check_rid;
int k = 0;

void init_bpm()
{
  BufferPoolManager::set_instance(&bpm);
}
void test_insert()
{
  RC rc = RC::SUCCESS;
  for (int i = 0; i < insert_num; i++) {

    rid.page_num = i / page_size;
    rid.slot_num = i % page_size;

    if (i % TIMES == 1) {
      if (insert_num > page_size) {
        if (k++ % 100 == 0) {
          LOG_INFO("Begin to insert the page's num %s", rid.to_string().c_str());
        }
      } else {
        LOG_INFO("Insert %d. rid=%s", i, rid.to_string().c_str());
      }
      rc = handler->insert_entry((const char *)&i, &rid);
      ASSERT_EQ(RC::SUCCESS, rc);
      handler->print_tree();
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
        LOG_INFO("Insert %d. rid=%s", i, rid.to_string().c_str());
      }
      rc = handler->insert_entry((const char *)&i, &rid);
      ASSERT_EQ(RC::SUCCESS, rc);
      handler->print_tree();
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
        LOG_INFO("Insert %d. rid=%s", i, rid.to_string().c_str());
      }
      rc = handler->insert_entry((const char *)&i, &rid);
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
      LOG_INFO("check duplicate Insert %d. rid=%s. i%TIMES=%d", i, rid.to_string().c_str(), i%TIMES);
    }
    rc = handler->insert_entry((const char *)&i, &rid);
    int t = i % TIMES;
    if (t == 0 || t == 1 || t == 2) {
      if (rc != RC::RECORD_DUPLICATE_KEY) {
	      LOG_WARN("insert duplicate key success");
      }
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
    RC rc = handler->get_entry((const char *)&i, 4, rids);

    ASSERT_EQ(RC::SUCCESS, rc);
    ASSERT_EQ(1, rids.size());
    check_rid = rids.front();
    ASSERT_EQ(rid.page_num, check_rid.page_num);
    ASSERT_EQ(rid.slot_num, check_rid.slot_num);
  }
}

void test_delete()
{
  RC rc = RC::SUCCESS;
  std::list<RID> rids;

  for (int i = 0; i < insert_num / 2; i++) {
    rid.page_num = i / page_size;
    rid.slot_num = i % page_size;

    int t = i % TIMES;
    if (t == 0 || t == 1) {
      if (insert_num > page_size) {
        if (k++ % 100 == 0) {
          LOG_INFO("Begin to delete entry of index, i=%d rid: %s", i, rid.to_string().c_str());
        }
      } else {
        LOG_INFO("Begin to delete entry of index,  i=%d, rid: %s", i, rid.to_string().c_str());
      }

      rc = handler->delete_entry((const char *)&i, &rid);
      if (rc != RC::SUCCESS) {
	      LOG_WARN("failed to delete entry. i=%d, rid=%s", i, rid.to_string().c_str());
      }
      ASSERT_EQ(RC::SUCCESS, rc);

      ASSERT_EQ(true, handler->validate_tree());
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
      rc = handler->delete_entry((const char *)&i, &rid);

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
        LOG_INFO("Begin to get entry of index, i=%d,rid: %s", i, rid.to_string().c_str());
      }
    } else {
      LOG_INFO("Begin to get entry of index, i=%d, rid: %s", i, rid.to_string().c_str());
    }
    rids.clear();
    rc = handler->get_entry((const char *)&i, 4, rids);
    ASSERT_EQ(RC::SUCCESS, rc);
    int t = i % TIMES;
    if (t == 0 || t == 1) {
      ASSERT_EQ(0, rids.size());
    } else {
      if (rids.size() != 1) {
	      LOG_WARN("invalid. i=%d, rid=%s, check rid=%s", i, rid.to_string().c_str(), check_rid.to_string().c_str());
      }
      ASSERT_EQ(1, rids.size());
      check_rid = rids.front();
      if (rid != check_rid) {
	      LOG_WARN("invalid. i=%d, rid=%s, check rid=%s", i, rid.to_string().c_str(), check_rid.to_string().c_str());
      }
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
      rc = handler->delete_entry((const char *)&i, &rid);

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
      rc = handler->delete_entry((const char *)&i, &rid);

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
    rc = handler->insert_entry((const char *)&i, &rid);
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

TEST(test_bplus_tree, test_leaf_index_node_handle)
{
  IndexFileHeader index_file_header;
  index_file_header.root_page = BP_INVALID_PAGE_NUM;
  index_file_header.internal_max_size = 5;
  index_file_header.leaf_max_size = 5;
  index_file_header.attr_length = 4;
  index_file_header.key_length = 4 + sizeof(RID);
  index_file_header.attr_type = INTS;

  Frame frame;

  KeyComparator key_comparator;
  key_comparator.init(INTS, 4);

  LeafIndexNodeHandler leaf_node(index_file_header, &frame);
  leaf_node.init_empty();
  ASSERT_EQ(0, leaf_node.size());

  bool found;
  int index;
  char key_mem[4 + sizeof(RID)];
  int &key = *(int *)key_mem;
  RID &rid = *(RID *)(key_mem + 4);
  rid.page_num = 0;
  rid.slot_num = 0;
  for (int i = 0; i < 5; i++) {
    key = i * 2 + 1;
    index = leaf_node.lookup(key_comparator, key_mem, &found);
    ASSERT_EQ(false, found);
    leaf_node.insert(index, (const char *)&key, (const char *)&rid);
  }

  ASSERT_EQ(5, leaf_node.size());

  for (int i = 0; i < 5; i++) {
    key = i * 2;
    index = leaf_node.lookup(key_comparator, key_mem, &found);
    ASSERT_EQ(false, found);
    ASSERT_EQ(i, index);
  }

  key = 12;
  index = leaf_node.lookup(key_comparator, key_mem, &found);
  ASSERT_EQ(false, found);
  ASSERT_EQ(5, index);

  for (int i = 0; i < 5; i++) {
    key = i * 2 + 1;
    index = leaf_node.lookup(key_comparator, key_mem, &found);
    if (!found || i != index) {
      printf("found=%d, index=%d, key=%d", found, index, key);
    }
    ASSERT_EQ(true, found);
    ASSERT_EQ(i, index);
  }
}
TEST(test_bplus_tree, test_internal_index_node_handle)
{
  IndexFileHeader index_file_header;
  index_file_header.root_page = BP_INVALID_PAGE_NUM;
  index_file_header.internal_max_size = 5;
  index_file_header.leaf_max_size = 5;
  index_file_header.attr_length = 4;
  index_file_header.key_length = 4 + sizeof(RID);
  index_file_header.attr_type = INTS;

  Frame frame;

  KeyComparator key_comparator;
  key_comparator.init(INTS, 4);

  InternalIndexNodeHandler internal_node(index_file_header, &frame);
  internal_node.init_empty();
  ASSERT_EQ(0, internal_node.size());

  bool found;
  int index;
  int insert_position;
  char key_mem[4 + sizeof(RID)];
  int &key = *(int *)key_mem;
  RID &rid = *(RID *)(key_mem + 4);
  rid.page_num = 0;
  rid.slot_num = 0;

  key = 0;
  index = internal_node.lookup(key_comparator, key_mem, &found, &insert_position);
  ASSERT_EQ(false, found);
  ASSERT_EQ(0, index);
  ASSERT_EQ(1, insert_position);

  key = 3;
  internal_node.create_new_root(1, key_mem, key);
  for (int i = 2; i < 5; i++) {
    key = i * 2 + 1;
    internal_node.insert((const char *)&key, (PageNum)key, key_comparator);
  }

  ASSERT_EQ(5, internal_node.size());

  for (int i = 1; i < 5; i++) {
    key = i * 2 + 1;
    int real_key = *(int*)internal_node.key_at(i);
    ASSERT_EQ(key, real_key);
  }

  key = 0;
  index = internal_node.lookup(key_comparator, key_mem, &found, &insert_position);
  ASSERT_EQ(false, found);
  ASSERT_EQ(0, index);
  ASSERT_EQ(1, insert_position);

  key = 2;
  index = internal_node.lookup(key_comparator, key_mem, &found, &insert_position);
  ASSERT_EQ(false, found);
  ASSERT_EQ(0, index);
  ASSERT_EQ(1, insert_position);

  key = 4;
  index = internal_node.lookup(key_comparator, key_mem, &found, &insert_position);
  ASSERT_EQ(false, found);
  ASSERT_EQ(1, index);
  ASSERT_EQ(2, insert_position);

  key = 8;
  index = internal_node.lookup(key_comparator, key_mem, &found, &insert_position);
  ASSERT_EQ(false, found);
  ASSERT_EQ(3, index);
  ASSERT_EQ(4, insert_position);

  key = 10;
  index = internal_node.lookup(key_comparator, key_mem, &found, &insert_position);
  ASSERT_EQ(false, found);
  ASSERT_EQ(4, index);
  ASSERT_EQ(5, insert_position);

  key = 12;
  index = internal_node.lookup(key_comparator, key_mem, &found);
  ASSERT_EQ(false, found);
  ASSERT_EQ(4, index);
  ASSERT_EQ(5, insert_position);

  for (int i = 1; i < 5; i++) {
    key = i * 2 + 1;
    index = internal_node.lookup(key_comparator, key_mem, &found);
    if (!found || i != index) {
      printf("found=%d, index=%d, key=%d", found, index, key);
    }
    ASSERT_EQ(true, found);
    ASSERT_EQ(i, index);
  }
}

TEST(test_bplus_tree, test_chars)
{
  LoggerFactory::init_default("test_chars.log");

  const char *index_name = "chars.btree";
  ::remove(index_name);
  handler = new BplusTreeHandler();
  handler->create(index_name, CHARS, 8, ORDER, ORDER);

  char keys[][9] = {
    "abcdefg",
    "12345678",
    "12345678",
    "abcdefg",
    "abcdefga"
  };

  RID rid;
  RC rc = RC::SUCCESS;
  for (size_t i = 0; i < sizeof(keys)/sizeof(keys[0]); i++) {
    rid.page_num = 0;
    rid.slot_num = i;
    rc = handler->insert_entry(keys[i], &rid);
    ASSERT_EQ(RC::SUCCESS, rc);
  }

  LOG_INFO("begin to print bplus tree of chars");
  handler->print_tree();
  LOG_INFO("end to print bplus tree of chars");

  BplusTreeScanner scanner(*handler);
  const char *key = "abcdefg";
  rc = scanner.open(key, strlen(key), true, key, strlen(key), true);
  ASSERT_EQ(rc, RC::SUCCESS);

  int count = 0;
  while (RC::SUCCESS == (rc = scanner.next_entry(rid))) {
    count++;
  }
  scanner.close();
  ASSERT_EQ(2, count);
}

TEST(test_bplus_tree, test_scanner)
{
  LoggerFactory::init_default("test.log");

  const char *index_name = "scanner.btree";
  ::remove(index_name);
  handler = new BplusTreeHandler();
  handler->create(index_name, INTS, sizeof(int), ORDER, ORDER);

  int count = 0;
  RC rc = RC::SUCCESS;
  RID rid;
  // 初始化B树，插入数据[1 - 199] 所有奇数
  for (int i = 0; i < 100; i++) {
    int key = i * 2 + 1;
    rid.page_num = 0;
    rid.slot_num = key;
    rc = handler->insert_entry((const char *)&key, &rid);
    ASSERT_EQ(RC::SUCCESS, rc);
  }

  handler->print_tree();

  BplusTreeScanner scanner(*handler);

  int begin = -100;
  int end = -20;
  rc = scanner.open((const char *)&begin, 4, false, (const char *)&end, 4, false);
  ASSERT_EQ(RC::SUCCESS, rc);

  rc = scanner.next_entry(rid);
  ASSERT_EQ(RC::RECORD_EOF, rc);

  scanner.close();

  begin = -100;
  end = 1;
  rc = scanner.open((const char *)&begin, 4, false, (const char *)&end, 4, false);
  ASSERT_EQ(RC::SUCCESS, rc);
  rc = scanner.next_entry(rid);
  ASSERT_EQ(RC::RECORD_EOF, rc);

  scanner.close();

  begin = -100;
  end = 1;
  rc = scanner.open((const char *)&begin, 4, false, (const char *)&end, 4, true/*inclusive*/);
  ASSERT_EQ(RC::SUCCESS, rc);
  rc = scanner.next_entry(rid);
  ASSERT_EQ(RC::SUCCESS, rc);
  rc = scanner.next_entry(rid);
  ASSERT_EQ(RC::RECORD_EOF, rc);

  scanner.close();

  begin = 1;
  end = 3;
  rc = scanner.open((const char *)&begin, 4, false, (const char *)&end, 4, false/*inclusive*/);
  ASSERT_EQ(RC::SUCCESS, rc);
  rc = scanner.next_entry(rid);
  ASSERT_EQ(RC::RECORD_EOF, rc);

  scanner.close();

  begin = 1;
  end = 3;
  rc = scanner.open((const char *)&begin, 4, true, (const char *)&end, 4, true/*inclusive*/);
  ASSERT_EQ(RC::SUCCESS, rc);
  while ((rc = scanner.next_entry(rid)) == RC::SUCCESS) {
    count++;
  }
  ASSERT_EQ(2, count);
  ASSERT_EQ(RC::RECORD_EOF, rc);

  scanner.close();

  begin = 0;
  end = 3;
  rc = scanner.open((const char *)&begin, 4, true, (const char *)&end, 4, true/*inclusive*/);
  ASSERT_EQ(RC::SUCCESS, rc);
  count = 0;
  while ((rc = scanner.next_entry(rid)) == RC::SUCCESS) {
    count++;
  }
  ASSERT_EQ(2, count);
  ASSERT_EQ(RC::RECORD_EOF, rc);

  scanner.close();

  begin = 11;
  end = 21;
  rc = scanner.open((const char *)&begin, 4, true, (const char *)&end, 4, true/*inclusive*/);
  ASSERT_EQ(RC::SUCCESS, rc);
  count = 0;
  while ((rc = scanner.next_entry(rid)) == RC::SUCCESS) {
    count++;
  }
  ASSERT_EQ((end - begin) / 2 + 1, count);
  ASSERT_EQ(RC::RECORD_EOF, rc);

  scanner.close();

  begin = 11;
  end = 91;
  rc = scanner.open((const char *)&begin, 4, true, (const char *)&end, 4, true/*inclusive*/);
  ASSERT_EQ(RC::SUCCESS, rc);
  count = 0;
  while ((rc = scanner.next_entry(rid)) == RC::SUCCESS) {
    count++;
  }
  ASSERT_EQ((end - begin) / 2 + 1, count);
  ASSERT_EQ(RC::RECORD_EOF, rc);

  scanner.close();

  begin = 191;
  end = 199;
  rc = scanner.open((const char *)&begin, 4, true, (const char *)&end, 4, true/*inclusive*/);
  ASSERT_EQ(RC::SUCCESS, rc);
  count = 0;
  while ((rc = scanner.next_entry(rid)) == RC::SUCCESS) {
    count++;
  }
  ASSERT_EQ((end - begin) / 2 + 1, count);
  ASSERT_EQ(RC::RECORD_EOF, rc);

  scanner.close();

  begin = 191;
  end = 201;
  rc = scanner.open((const char *)&begin, 4, true, (const char *)&end, 4, true/*inclusive*/);
  ASSERT_EQ(RC::SUCCESS, rc);
  count = 0;
  while ((rc = scanner.next_entry(rid)) == RC::SUCCESS) {
    count++;
  }
  ASSERT_EQ(5, count);
  ASSERT_EQ(RC::RECORD_EOF, rc);

  scanner.close();

  begin = 200;
  end = 301;
  rc = scanner.open((const char *)&begin, 4, true, (const char *)&end, 4, true/*inclusive*/);
  ASSERT_EQ(RC::SUCCESS, rc);
  rc = scanner.next_entry(rid);
  ASSERT_EQ(RC::RECORD_EOF, rc);

  scanner.close();

  begin = 300;
  end = 201;
  rc = scanner.open((const char *)&begin, 4, true, (const char *)&end, 4, true/*inclusive*/);
  ASSERT_EQ(RC::INVALID_ARGUMENT, rc);

  scanner.close();

  begin = 300;
  end = 201;
  rc = scanner.open(nullptr, 4, true, (const char *)&end, 4, true/*inclusive*/);
  ASSERT_EQ(RC::SUCCESS, rc);
  count = 0;
  while ((rc = scanner.next_entry(rid)) == RC::SUCCESS) {
    count++;
  }
  ASSERT_EQ(100, count);
  ASSERT_EQ(RC::RECORD_EOF, rc);

  scanner.close();

  begin = 300;
  end = 10;
  rc = scanner.open(nullptr, 4, true, (const char *)&end, 4, true/*inclusive*/);
  ASSERT_EQ(RC::SUCCESS, rc);
  count = 0;
  while ((rc = scanner.next_entry(rid)) == RC::SUCCESS) {
    count++;
  }
  ASSERT_EQ(5, count);
  ASSERT_EQ(RC::RECORD_EOF, rc);

  scanner.close();

  begin = 190;
  end = 10;
  rc = scanner.open((const char *)&begin, 4, true, nullptr, 4, true/*inclusive*/);
  ASSERT_EQ(RC::SUCCESS, rc);
  count = 0;
  while ((rc = scanner.next_entry(rid)) == RC::SUCCESS) {
    count++;
  }
  ASSERT_EQ(5, count);
  ASSERT_EQ(RC::RECORD_EOF, rc);

  scanner.close();

  begin = 190;
  end = 10;
  rc = scanner.open(nullptr, 0, true, nullptr, 0, true/*inclusive*/);
  ASSERT_EQ(RC::SUCCESS, rc);
  count = 0;
  while ((rc = scanner.next_entry(rid)) == RC::SUCCESS) {
    count++;
  }
  ASSERT_EQ(100, count);
  ASSERT_EQ(RC::RECORD_EOF, rc);

  scanner.close();
}

TEST(test_bplus_tree, test_bplus_tree_insert)
{
  LoggerFactory::init_default("test.log");

  ::remove(index_name);
  handler = new BplusTreeHandler();
  handler->create(index_name, INTS, sizeof(int), ORDER, ORDER);

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

    LoggerFactory::init_default("test.log", LOG_LEVEL_TRACE);
  init_bpm();
  int rc = RUN_ALL_TESTS();

  return rc;
}
