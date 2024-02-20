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
// Created by wangyunlai.wyl on 2024/02/20.
//

#include "storage/index/bplus_tree_log_entry.h"
#include "common/lang/serialize_buffer.h"

using namespace std;
using namespace common;

namespace bplus_tree {

string LogOperation::to_string() const
{
  stringstream ss;
  ss << std::to_string(index()) << ":";
  switch (type_) {
    case Type::INIT_HEADER_PAGE: ss << "INIT_HEADER_PAGE"; break;
    default: ss << "UNKNOWN"; break;
  }
  return ss.str();
}

///////////////////////////////////////////////////////////////////////////////
// LogEntry
RC LogEntry::serialize(SerializeBuffer &buffer) const
{
  RC rc = serialize_header(buffer);
  if (OB_FAIL(rc)) {
    return rc;
  }
  return serialize_body(buffer);
}

RC LogEntry::serialize_header(SerializeBuffer &buffer) const
{
  int32_t type = this->operation_type().index();

  int ret = buffer.write_int32(type);
  if (ret < 0) {
    return RC::INTERNAL;
  }
  return RC::SUCCESS;
}

RC InitHeaderPageLogEntry::serialize_body(common::SerializeBuffer &buffer) const
{
  buffer.write_int32(attr_length_);
  buffer.write_int32(attr_type_);
  buffer.write_int32(internal_max_size_);
  buffer.write_int32(leaf_max_size_);
  return RC::SUCCESS;
}

RC SetParentPageLogEntry::serialize_body(SerializeBuffer &buffer) const
{
  int ret = buffer.write_int32(parent_page_num_);
  return ret == 0 ? RC::SUCCESS : RC::INTERNAL;
}

RC NormalOperationLogEntry::serialize_body(SerializeBuffer &buffer) const
{
  int ret = 0;
  if ((ret = buffer.write_int32(index_)) < 0 || (ret = buffer.write_int32(item_num_) < 0) ||
      (ret = buffer.write(items_) < 0)) {
    return RC::INTERNAL;
  }

  return RC::SUCCESS;
}

RC LeafSetNextPageLogEntry::serialize_body(common::SerializeBuffer &buffer) const
{
  buffer.write_int32(new_page_num_);
  return RC::SUCCESS;
}

RC InternalCreateNewRootLogEntry::serialize_body(common::SerializeBuffer &buffer) const
{
  buffer.write_int32(first_page_num_);
  buffer.write_int32(page_num_);
  buffer.write(key_);
  return RC::SUCCESS;
}

RC InternalUpdateKeyLogEntry::serialize_body(common::SerializeBuffer &buffer) const
{
  buffer.write_int32(index_);
  buffer.write(key_);
  buffer.write(old_key_);
  return RC::SUCCESS;
}

RC UpdateRootPageLogEntry::serialize_body(common::SerializeBuffer &buffer) const
{
  buffer.write_int32(root_page_num_);
  return RC::SUCCESS;
}

RC InitHeaderPageLogEntry::rollback(BplusTreeMiniTransaction &, BplusTreeHandler &)
{
  // do nothing
  return RC::SUCCESS;
}

RC UpdateRootPageLogEntry::rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler)
{
  return tree_handler.rocover_update_root_page(mtr, old_page_num_);
}

RC SetParentPageLogEntry::rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &)
{
  return node_handler_.set_parent_page_num(old_parent_page_num_);
}

RC InternalUpdateKeyLogEntry::rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler)
{
  InternalIndexNodeHandler &internal_handler = static_cast<InternalIndexNodeHandler &>(node_handler_);
  internal_handler.set_key_at(index_, old_key_.data());
  return RC::SUCCESS;
}

RC LeafSetNextPageLogEntry::rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &)
{
  LeafIndexNodeHandler &leaf_handler = static_cast<LeafIndexNodeHandler &>(node_handler_);
  leaf_handler.set_next_page(old_page_num_);
  return RC::SUCCESS;
}

RC NormalOperationLogEntry::rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &)
{
  if (operation_type().type() == LogOperation::Type::NODE_INSERT) {
    return node_handler_.recover_remove_items(index_, item_num_);
  } else {  // should be NODE_REMOVE
    return node_handler_.recover_insert_items(index_, items_.data(), item_num_);
  }
}

}  // namespace bplus_tree