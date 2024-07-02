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

#include <filesystem>

#include "gtest/gtest.h"
#include "storage/index/bplus_tree_log_entry.h"
#include "common/lang/serializer.h"

using namespace std;
using namespace common;
using namespace bplus_tree;

TEST(BplusTreeLogEntry, init_header_page_log_entry)
{
  IndexFileHeader file_header;
  file_header.root_page         = -1;
  file_header.internal_max_size = 100;
  file_header.leaf_max_size     = 200;
  file_header.attr_length       = 20;
  file_header.key_length        = 30;
  file_header.attr_type         = AttrType::INTS;

  Frame frame;
  frame.set_page_num(100);
  InitHeaderPageLogEntryHandler init_header_page_entry(&frame, file_header);

  Serializer serializer;
  ASSERT_EQ(RC::SUCCESS, init_header_page_entry.serialize(serializer));

  Deserializer                deserializer(serializer.data());
  unique_ptr<LogEntryHandler> handler;

  ASSERT_EQ(RC::SUCCESS, LogEntryHandler::from_buffer(deserializer, handler));

  InitHeaderPageLogEntryHandler *init_header_page_entry2 = dynamic_cast<InitHeaderPageLogEntryHandler *>(handler.get());
  ASSERT_NE(nullptr, init_header_page_entry2);
  const IndexFileHeader &file_header2 = init_header_page_entry2->file_header();
  ASSERT_EQ(file_header.root_page, file_header2.root_page);
  ASSERT_EQ(file_header.internal_max_size, file_header2.internal_max_size);
  ASSERT_EQ(file_header.leaf_max_size, file_header2.leaf_max_size);
  ASSERT_EQ(file_header.attr_length, file_header2.attr_length);
  ASSERT_EQ(file_header.key_length, file_header2.key_length);
  ASSERT_EQ(file_header.attr_type, file_header2.attr_type);
}

TEST(BplusTreeLogEntry, update_root_page_log_entry)
{
  Frame frame;
  frame.set_page_num(100);
  PageNum                       root_page_num     = 1000;
  PageNum                       old_root_page_num = 200;
  UpdateRootPageLogEntryHandler entry(&frame, root_page_num, old_root_page_num);

  // test serializer and desirializer
  Serializer serializer;
  ASSERT_EQ(RC::SUCCESS, entry.serialize(serializer));

  Deserializer                deserializer(serializer.data());
  unique_ptr<LogEntryHandler> handler;
  ASSERT_EQ(RC::SUCCESS, LogEntryHandler::from_buffer(deserializer, handler));

  UpdateRootPageLogEntryHandler *entry2 = dynamic_cast<UpdateRootPageLogEntryHandler *>(handler.get());
  ASSERT_EQ(root_page_num, entry2->root_page_num());
}

TEST(BplusTreeLogEntry, set_parent_page_log_entry)
{
  Frame frame;
  frame.set_page_num(100);
  PageNum                      parent_page_num     = 1000;
  PageNum                      old_parent_page_num = 200;
  SetParentPageLogEntryHandler entry(&frame, parent_page_num, old_parent_page_num);

  // test serializer and desirializer
  Serializer serializer;
  ASSERT_EQ(RC::SUCCESS, entry.serialize(serializer));

  Deserializer                deserializer(serializer.data());
  unique_ptr<LogEntryHandler> handler;
  ASSERT_EQ(RC::SUCCESS, LogEntryHandler::from_buffer(deserializer, handler));

  SetParentPageLogEntryHandler *entry2 = dynamic_cast<SetParentPageLogEntryHandler *>(handler.get());
  ASSERT_EQ(parent_page_num, entry2->parent_page_num());
}

TEST(BplusTreeLogEntry, normal_operation_log_entry)
{
  Frame frame;
  frame.set_page_num(100);
  LogOperation                   operation = LogOperation::Type::NODE_INSERT;
  vector<char>                   insert_items(100);
  int                            insert_index = 10;
  int                            item_num     = 5;
  NormalOperationLogEntryHandler entry(&frame, operation, insert_index, insert_items, item_num);

  // test serializer and desirializer
  Serializer serializer;
  ASSERT_EQ(RC::SUCCESS, entry.serialize(serializer));

  Deserializer                deserializer(serializer.data());
  unique_ptr<LogEntryHandler> handler;
  ASSERT_EQ(RC::SUCCESS, LogEntryHandler::from_buffer(deserializer, handler));

  auto entry2 = dynamic_cast<NormalOperationLogEntryHandler *>(handler.get());
  ASSERT_EQ(operation.type(), entry2->operation_type().type());
  ASSERT_EQ(insert_index, entry2->index());
  ASSERT_EQ(item_num, entry2->item_num());
  ASSERT_EQ(insert_items.size(), entry2->item_bytes());
  ASSERT_EQ(0, memcmp(insert_items.data(), entry2->items(), insert_items.size()));
}

TEST(BplusTreeLogEntry, leaf_init_empty_log_entry)
{
  Frame frame;
  frame.set_page_num(100);
  LeafInitEmptyLogEntryHandler entry(&frame);

  // test serializer and desirializer
  Serializer serializer;
  ASSERT_EQ(RC::SUCCESS, entry.serialize(serializer));

  Deserializer                deserializer(serializer.data());
  unique_ptr<LogEntryHandler> handler;
  ASSERT_EQ(RC::SUCCESS, LogEntryHandler::from_buffer(deserializer, handler));

  auto entry2 = dynamic_cast<LeafInitEmptyLogEntryHandler *>(handler.get());
  ASSERT_NE(nullptr, entry2);
}

TEST(BplusTreeLogEntry, leaf_set_next_page_log_entry)
{
  Frame frame;
  frame.set_page_num(100);
  PageNum                        next_page_num     = 1000;
  PageNum                        old_next_page_num = 200;
  LeafSetNextPageLogEntryHandler entry(&frame, next_page_num, old_next_page_num);

  // test serializer and desirializer
  Serializer serializer;
  ASSERT_EQ(RC::SUCCESS, entry.serialize(serializer));

  Deserializer                deserializer(serializer.data());
  unique_ptr<LogEntryHandler> handler;
  ASSERT_EQ(RC::SUCCESS, LogEntryHandler::from_buffer(deserializer, handler));

  auto entry2 = dynamic_cast<LeafSetNextPageLogEntryHandler *>(handler.get());
  ASSERT_NE(nullptr, entry2);
  ASSERT_EQ(next_page_num, entry2->new_page_num());
}

TEST(BplusTreeLogEntry, internal_init_empty_log_entry)
{
  Frame frame;
  frame.set_page_num(100);
  InternalInitEmptyLogEntryHandler entry(&frame);

  // test serializer and desirializer
  Serializer serializer;
  ASSERT_EQ(RC::SUCCESS, entry.serialize(serializer));

  Deserializer                deserializer(serializer.data());
  unique_ptr<LogEntryHandler> handler;
  ASSERT_EQ(RC::SUCCESS, LogEntryHandler::from_buffer(deserializer, handler));

  auto entry2 = dynamic_cast<InternalInitEmptyLogEntryHandler *>(handler.get());
  ASSERT_NE(nullptr, entry2);
}

TEST(BplusTreeLogEntry, internal_create_new_root_log_entry)
{
  Frame frame;
  frame.set_page_num(100);
  PageNum                              first_page_num = 1000;
  PageNum                              page_num       = 200;
  vector<char>                         key(100);
  InternalCreateNewRootLogEntryHandler entry(&frame, first_page_num, key, page_num);

  // test serializer and desirializer
  Serializer serializer;
  ASSERT_EQ(RC::SUCCESS, entry.serialize(serializer));

  Deserializer                deserializer(serializer.data());
  unique_ptr<LogEntryHandler> handler;
  ASSERT_EQ(RC::SUCCESS, LogEntryHandler::from_buffer(deserializer, handler));

  auto entry2 = dynamic_cast<InternalCreateNewRootLogEntryHandler *>(handler.get());
  ASSERT_NE(nullptr, entry2);
  ASSERT_EQ(first_page_num, entry2->first_page_num());
  ASSERT_EQ(page_num, entry2->page_num());
  ASSERT_EQ(key.size(), entry2->key_bytes());
  ASSERT_EQ(0, memcmp(key.data(), entry2->key(), key.size()));
}

TEST(BplusTreeLogEntry, internal_update_key_log_entry)
{
  Frame frame;
  frame.set_page_num(100);
  int                              update_index = 20;
  vector<char>                     key(100);
  vector<char>                     old_key(100);
  InternalUpdateKeyLogEntryHandler entry(&frame, update_index, key, old_key);

  // test serializer and desirializer
  Serializer serializer;
  ASSERT_EQ(RC::SUCCESS, entry.serialize(serializer));

  Deserializer                deserializer(serializer.data());
  unique_ptr<LogEntryHandler> handler;
  ASSERT_EQ(RC::SUCCESS, LogEntryHandler::from_buffer(deserializer, handler));

  auto entry2 = dynamic_cast<InternalUpdateKeyLogEntryHandler *>(handler.get());
  ASSERT_NE(nullptr, entry2);
  ASSERT_EQ(update_index, entry2->index());
  ASSERT_EQ(key.size(), entry2->key_bytes());
  ASSERT_EQ(0, memcmp(key.data(), entry2->key(), key.size()));
}

int main(int argc, char **argv)
{
  testing::InitGoogleTest(&argc, argv);
  filesystem::path log_filename = filesystem::path(argv[0]).filename();
  LoggerFactory::init_default(log_filename.string() + ".log", LOG_LEVEL_TRACE);
  return RUN_ALL_TESTS();
}
