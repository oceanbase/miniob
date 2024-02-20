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
// Created by wangyunlai.wyl on 2024/02/05.
//

#include <sstream>
#include <ranges>

#include "storage/index/bplus_tree_log.h"
#include "storage/index/bplus_tree.h"
#include "storage/clog/log_handler.h"
#include "storage/index/bplus_tree_log_entry.h"
#include "common/lang/serialize_buffer.h"

using namespace std;
using namespace common;
using namespace bplus_tree;

///////////////////////////////////////////////////////////////////////////////
// class BplusTreeLogger
BplusTreeLogger::BplusTreeLogger(LogHandler &log_handler, int32_t buffer_pool_id)
  : log_handler_(log_handler),
    buffer_pool_id_(buffer_pool_id)
{}

BplusTreeLogger::~BplusTreeLogger()
{
  commit();
}

RC BplusTreeLogger::init_header_page(Frame *frame, int32_t attr_length, int32_t attr_type, int32_t internal_max_size, int32_t leaf_max_size)
{
  return append_log_entry(make_unique<InitHeaderPageLogEntry>(frame, attr_length, attr_type, internal_max_size, leaf_max_size));
}

RC BplusTreeLogger::update_root_page(Frame *frame, PageNum root_page_num, PageNum old_page_num)
{
  return append_log_entry(make_unique<UpdateRootPageLogEntry>(frame, root_page_num, old_page_num));
}

RC BplusTreeLogger::leaf_init_empty(IndexNodeHandler &node_handler)
{
  return append_log_entry(make_unique<LeafInitEmptyLogEntry>(node_handler));
}

RC BplusTreeLogger::node_insert_items(IndexNodeHandler &node_handler, int index, span<const char> items, int item_num)
{
  return append_log_entry(make_unique<NormalOperationLogEntry>(node_handler, LogOperation::Type::NODE_INSERT, index, items, item_num));
}

RC BplusTreeLogger::node_remove_items(IndexNodeHandler &node_handler, int index, span<const char> items, int item_num)
{
  return append_log_entry(make_unique<NormalOperationLogEntry>(node_handler, LogOperation::Type::NODE_REMOVE, index, items, item_num));
}

RC BplusTreeLogger::leaf_set_next_page(IndexNodeHandler &node_handler, PageNum page_num, PageNum old_page_num)
{
  return append_log_entry(make_unique<LeafSetNextPageLogEntry>(node_handler, page_num, old_page_num));
}

RC BplusTreeLogger::internal_init_empty(IndexNodeHandler &node_handler)
{
  return append_log_entry(make_unique<InternalInitEmptyLogEntry>(node_handler));
}

RC BplusTreeLogger::internal_create_new_root(IndexNodeHandler &node_handler, PageNum first_page_num, span<const char> key, PageNum page_num)
{
  return append_log_entry(make_unique<InternalCreateNewRootLogEntry>(node_handler, first_page_num, key, page_num));
}

RC BplusTreeLogger::internal_update_key(IndexNodeHandler &node_handler, int index, span<const char> key, span<const char> old_key)
{
  return append_log_entry(make_unique<InternalUpdateKeyLogEntry>(node_handler, index, key, old_key));
}

RC BplusTreeLogger::set_parent_page(IndexNodeHandler &node_handler, PageNum page_num, PageNum old_page_num)
{
  return append_log_entry(make_unique<SetParentPageLogEntry>(node_handler, page_num, old_page_num));
}

RC BplusTreeLogger::append_log_entry(unique_ptr<bplus_tree::LogEntry> entry)
{
  if (rolling_back_) {
    return RC::SUCCESS;
  }

  entries_.push_back(std::move(entry));
  return RC::SUCCESS;
}

RC BplusTreeLogger::commit()
{
  LSN lsn = 0;
  SerializeBuffer buffer;
  buffer.write_int32(buffer_pool_id_);

  for (auto &entry : entries_) {
    entry->serialize(buffer);
  }

  SerializeBuffer::BufferType &buffer_data = buffer.data();
  RC rc = log_handler_.append(lsn, LogModule::Id::BPLUS_TREE, std::move(buffer_data));
  if (RC::SUCCESS != rc) {
    LOG_WARN("failed to append log entry. rc=%s", strrc(rc));
    return rc;
  }

  for (auto &entry : entries_) {
    entry->frame()->set_lsn(lsn);
  }

  entries_.clear();
  return RC::SUCCESS;
}

RC BplusTreeLogger::rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler)
{
  rolling_back_ = true;

  for (auto &entry : ranges::reverse_view(entries_)) {
    entry->rollback(mtr, tree_handler);
  }

  entries_.clear();
  rolling_back_ = false;
  return RC::SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// class BplusTreeMiniTransaction
BplusTreeMiniTransaction::BplusTreeMiniTransaction(BplusTreeHandler &tree_handler, RC *operation_result /* =nullptr */)
  : tree_handler_(tree_handler),
    operation_result_(operation_result),
    latch_memo_(&tree_handler.buffer_pool()),
    logger_(tree_handler.log_handler(), tree_handler.buffer_pool().id())
{}

BplusTreeMiniTransaction::~BplusTreeMiniTransaction()
{
  if (operation_result_ && OB_SUCC(*operation_result_)) {
    commit();
  } else {
    rollback();
  }
}

RC BplusTreeMiniTransaction::commit()
{
  return logger_.commit();
}

RC BplusTreeMiniTransaction::rollback()
{
  return logger_.rollback(*this, tree_handler_);
}
