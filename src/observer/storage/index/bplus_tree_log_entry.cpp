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
#include "common/lang/serializer.h"

using namespace common;

namespace bplus_tree {

string LogOperation::to_string() const
{
  stringstream ss;
  ss << std::to_string(index()) << ":";
  switch (type_) {
    case Type::INIT_HEADER_PAGE: ss << "INIT_HEADER_PAGE"; break;
    case Type::UPDATE_ROOT_PAGE: ss << "UPDATE_ROOT_PAGE"; break;
    case Type::SET_PARENT_PAGE: ss << "SET_PARENT_PAGE"; break;
    case Type::LEAF_INIT_EMPTY: ss << "LEAF_INIT_EMPTY"; break;
    case Type::LEAF_SET_NEXT_PAGE: ss << "LEAF_SET_NEXT_PAGE"; break;
    case Type::INTERNAL_INIT_EMPTY: ss << "INTERNAL_INIT_EMPTY"; break;
    case Type::INTERNAL_CREATE_NEW_ROOT: ss << "INTERNAL_CREATE_NEW_ROOT"; break;
    case Type::INTERNAL_UPDATE_KEY: ss << "INTERNAL_UPDATE_KEY"; break;
    case Type::NODE_INSERT: ss << "NODE_INSERT"; break;
    case Type::NODE_REMOVE: ss << "NODE_REMOVE"; break;
    default: ss << "INVALID"; break;
  }
  return ss.str();
}

///////////////////////////////////////////////////////////////////////////////
// LogEntry
LogEntryHandler::LogEntryHandler(LogOperation operation, Frame *frame) : operation_type_(operation), frame_(frame)
{
  if (frame_ != nullptr) {
    set_page_num(frame->page_num());
  }
}

PageNum LogEntryHandler::page_num() const
{
  if (frame_ != nullptr) {
    return frame_->page_num();
  }
  return page_num_;
}

RC LogEntryHandler::serialize(Serializer &buffer) const
{
  RC rc = serialize_header(buffer);
  if (OB_FAIL(rc)) {
    return rc;
  }
  return serialize_body(buffer);
}

RC LogEntryHandler::serialize_header(Serializer &buffer) const
{
  int32_t type     = this->operation_type().index();
  PageNum page_num = frame_->page_num();

  int ret = buffer.write_int32(type);
  if (ret < 0) {
    return RC::INTERNAL;
  }
  ret = buffer.write_int32(page_num);
  if (ret < 0) {
    return RC::INTERNAL;
  }
  return RC::SUCCESS;
}

string LogEntryHandler::to_string() const
{
  stringstream ss;
  ss << "operation=" << operation_type().to_string() << ", page_num=" << page_num();
  return ss.str();
}

RC LogEntryHandler::from_buffer(Deserializer &deserializer, unique_ptr<LogEntryHandler> &handler)
{
  auto fake_frame_getter = [](PageNum, Frame *&frame) -> RC {
    frame = nullptr;
    return RC::SUCCESS;
  };
  return from_buffer(fake_frame_getter, deserializer, handler);
}

RC LogEntryHandler::from_buffer(
    DiskBufferPool &buffer_pool, common::Deserializer &buffer, unique_ptr<LogEntryHandler> &handler)
{
  auto frame_getter = [&buffer_pool](PageNum page_num, Frame *&frame) -> RC {
    return buffer_pool.get_this_page(page_num, &frame);
  };
  return from_buffer(frame_getter, buffer, handler);
}

RC LogEntryHandler::from_buffer(
    function<RC(PageNum, Frame *&)> frame_getter, Deserializer &buffer, unique_ptr<LogEntryHandler> &handler)
{
  int32_t type     = -1;
  PageNum page_num = -1;
  int     ret      = buffer.read_int32(type);
  if (ret != 0) {
    return RC::INVALID_ARGUMENT;
  }

  if (type < 0 || type >= static_cast<int32_t>(LogOperation::Type::MAX_TYPE)) {
    return RC::INVALID_ARGUMENT;
  }

  ret = buffer.read_int32(page_num);
  if (ret != 0) {
    return RC::INVALID_ARGUMENT;
  }

  Frame *frame = nullptr;
  RC     rc    = frame_getter(page_num, frame);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to get frame. page_num=%d, rc=%s", page_num, strrc(rc));
    return rc;
  }

  LogOperation operation(type);

  switch (operation.type()) {
    case LogOperation::Type::INIT_HEADER_PAGE: {
      rc = InitHeaderPageLogEntryHandler::deserialize(frame, buffer, handler);
    } break;

    case LogOperation::Type::UPDATE_ROOT_PAGE: {
      rc = UpdateRootPageLogEntryHandler::deserialize(frame, buffer, handler);
    } break;

    case LogOperation::Type::SET_PARENT_PAGE: {
      rc = SetParentPageLogEntryHandler::deserialize(frame, buffer, handler);
    } break;

    case LogOperation::Type::LEAF_INIT_EMPTY: {
      rc = LeafInitEmptyLogEntryHandler::deserialize(frame, buffer, handler);
    } break;

    case LogOperation::Type::LEAF_SET_NEXT_PAGE: {
      rc = LeafSetNextPageLogEntryHandler::deserialize(frame, buffer, handler);
    } break;

    case LogOperation::Type::INTERNAL_INIT_EMPTY: {
      rc = InternalInitEmptyLogEntryHandler::deserialize(frame, buffer, handler);
    } break;

    case LogOperation::Type::INTERNAL_CREATE_NEW_ROOT: {
      rc = InternalCreateNewRootLogEntryHandler::deserialize(frame, buffer, handler);
    } break;

    case LogOperation::Type::INTERNAL_UPDATE_KEY: {
      rc = InternalUpdateKeyLogEntryHandler::deserialize(frame, buffer, handler);
    } break;

    case LogOperation::Type::NODE_INSERT:
    case LogOperation::Type::NODE_REMOVE: {
      rc = NormalOperationLogEntryHandler::deserialize(frame, operation, buffer, handler);
    } break;

    default: {
      LOG_ERROR("unknown log operation. operation=%d:%s", operation.index(), operation.to_string().c_str());
      return RC::INTERNAL;
    }
  }

  if (OB_SUCC(rc) && handler) {
    handler->set_page_num(page_num);
  }
  return rc;
}

///////////////////////////////////////////////////////////////////////////////
// InitHeaderPageLogEntryHandler
InitHeaderPageLogEntryHandler::InitHeaderPageLogEntryHandler(Frame *frame, const IndexFileHeader &file_header)
    : LogEntryHandler(LogOperation::Type::INIT_HEADER_PAGE, frame), file_header_(file_header)
{}

RC InitHeaderPageLogEntryHandler::serialize_body(Serializer &buffer) const
{
  buffer.write(span<const char>(reinterpret_cast<const char *>(&file_header_), sizeof(file_header_)));
  return RC::SUCCESS;
}

string InitHeaderPageLogEntryHandler::to_string() const
{
  stringstream ss;
  ss << LogEntryHandler::to_string() << ", file_header=" << file_header_.to_string();
  return ss.str();
}

RC InitHeaderPageLogEntryHandler::deserialize(Frame *frame, Deserializer &buffer, unique_ptr<LogEntryHandler> &handler)
{
  IndexFileHeader header;
  int             ret = buffer.read(span<char>(reinterpret_cast<char *>(&header), sizeof(header)));
  if (ret != 0) {
    return RC::INTERNAL;
  }

  handler = make_unique<InitHeaderPageLogEntryHandler>(frame, header);
  return RC::SUCCESS;
}

RC InitHeaderPageLogEntryHandler::rollback(BplusTreeMiniTransaction &, BplusTreeHandler &)
{
  // do nothing
  return RC::SUCCESS;
}

RC InitHeaderPageLogEntryHandler::redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler)
{
  return tree_handler.recover_init_header_page(mtr, frame(), file_header_);
}

///////////////////////////////////////////////////////////////////////////////
// SetParentPageLogEntryHandler
SetParentPageLogEntryHandler::SetParentPageLogEntryHandler(
    Frame *frame, PageNum parent_page_num, PageNum old_parent_page_num)
    : NodeLogEntryHandler(LogOperation::Type::SET_PARENT_PAGE, frame),
      parent_page_num_(parent_page_num),
      old_parent_page_num_(old_parent_page_num)
{}

RC SetParentPageLogEntryHandler::serialize_body(Serializer &buffer) const
{
  int ret = buffer.write_int32(parent_page_num_);
  return ret == 0 ? RC::SUCCESS : RC::INTERNAL;
}

string SetParentPageLogEntryHandler::to_string() const
{
  stringstream ss;
  ss << LogEntryHandler::to_string() << ", parent_page_num=" << parent_page_num_;
  return ss.str();
}

RC SetParentPageLogEntryHandler::deserialize(Frame *frame, Deserializer &buffer, unique_ptr<LogEntryHandler> &handler)
{
  int     ret             = 0;
  int32_t parent_page_num = -1;
  if ((ret = buffer.read_int32(parent_page_num)) < 0) {
    return RC::INTERNAL;
  }

  handler = make_unique<SetParentPageLogEntryHandler>(frame, parent_page_num, -1 /*old_parent_page_num*/);
  return RC::SUCCESS;
}

RC SetParentPageLogEntryHandler::rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler)
{
  if (nullptr == frame()) {
    return RC::INTERNAL;
  }
  IndexNodeHandler node_handler(mtr, tree_handler.file_header(), frame());
  return node_handler.set_parent_page_num(old_parent_page_num_);
}

RC SetParentPageLogEntryHandler::redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler)
{
  IndexNodeHandler node_handler(mtr, tree_handler.file_header(), frame());
  return node_handler.set_parent_page_num(parent_page_num_);
}

///////////////////////////////////////////////////////////////////////////////
// NormalOperationLogEntryHandler
NormalOperationLogEntryHandler::NormalOperationLogEntryHandler(
    Frame *frame, LogOperation operation, int index, span<const char> items, int item_num)
    : NodeLogEntryHandler(operation, frame), index_(index), item_num_(item_num), items_(items.begin(), items.end())
{}

RC NormalOperationLogEntryHandler::serialize_body(Serializer &buffer) const
{
  int     ret        = 0;
  int32_t item_bytes = static_cast<int32_t>(items_.size());
  if ((ret = buffer.write_int32(index_)) < 0 || (ret = buffer.write_int32(item_num_) < 0) ||
      (ret = buffer.write_int32(item_bytes) < 0) || (ret = buffer.write(items_) < 0)) {
    return RC::INTERNAL;
  }

  return RC::SUCCESS;
}

string NormalOperationLogEntryHandler::to_string() const
{
  stringstream ss;
  ss << LogEntryHandler::to_string() << ", index=" << index_ << ", item_num=" << item_num_;
  return ss.str();
}

RC NormalOperationLogEntryHandler::deserialize(
    Frame *frame, LogOperation operation, Deserializer &buffer, unique_ptr<LogEntryHandler> &handler)
{
  int ret = 0;

  int32_t index      = -1;
  int32_t item_num   = -1;
  int32_t item_bytes = -1;
  if ((ret = buffer.read_int32(index)) < 0 || (ret = buffer.read_int32(item_num)) < 0 ||
      (ret = buffer.read_int32(item_bytes)) < 0) {
    return RC::INTERNAL;
  }

  vector<char> items(item_bytes);
  if ((ret = buffer.read(items)) < 0) {
    return RC::INTERNAL;
  }

  handler = make_unique<NormalOperationLogEntryHandler>(frame, operation.type(), index, items, item_num);
  return RC::SUCCESS;
}

RC NormalOperationLogEntryHandler::rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler)
{
  if (nullptr == frame()) {
    return RC::INTERNAL;
  }
  IndexNodeHandler node_handler(mtr, tree_handler.file_header(), frame());
  if (operation_type().type() == LogOperation::Type::NODE_INSERT) {
    return node_handler.recover_remove_items(index_, item_num_);
  } else {  // should be NODE_REMOVE
    return node_handler.recover_insert_items(index_, items_.data(), item_num_);
  }
}

RC NormalOperationLogEntryHandler::redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler)
{
  InternalIndexNodeHandler internal_node(mtr, tree_handler.file_header(), frame());
  LeafIndexNodeHandler     leaf_node(mtr, tree_handler.file_header(), frame());
  IndexNodeHandler         node_handler(mtr, tree_handler.file_header(), frame());
  IndexNodeHandler        *real_handler = nullptr;
  if (node_handler.is_leaf()) {
    real_handler = &leaf_node;
  } else {
    real_handler = &internal_node;
  }
  if (operation_type().type() == LogOperation::Type::NODE_INSERT) {
    return real_handler->recover_insert_items(index_, items_.data(), item_num_);
  } else {  // should be NODE_REMOVE
    return real_handler->recover_remove_items(index_, item_num_);
  }
}

///////////////////////////////////////////////////////////////////////////////
// LeafInitEmptyLogEntryHandler
LeafInitEmptyLogEntryHandler::LeafInitEmptyLogEntryHandler(Frame *frame)
    : NodeLogEntryHandler(LogOperation::Type::LEAF_INIT_EMPTY, frame)
{}

RC LeafInitEmptyLogEntryHandler::redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler)
{
  LeafIndexNodeHandler leaf_handler(mtr, tree_handler.file_header(), frame());
  RC rc = leaf_handler.init_empty();
  return rc;
}

RC LeafInitEmptyLogEntryHandler::deserialize(Frame *frame, Deserializer &buffer, unique_ptr<LogEntryHandler> &handler)
{
  handler = make_unique<LeafInitEmptyLogEntryHandler>(frame);
  return RC::SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// LeafSetNextPageLogEntryHandler
LeafSetNextPageLogEntryHandler::LeafSetNextPageLogEntryHandler(Frame *frame, PageNum new_page_num, PageNum old_page_num)
    : NodeLogEntryHandler(LogOperation::Type::LEAF_SET_NEXT_PAGE, frame),
      new_page_num_(new_page_num),
      old_page_num_(old_page_num)
{}

RC LeafSetNextPageLogEntryHandler::serialize_body(Serializer &buffer) const
{
  buffer.write_int32(new_page_num_);
  return RC::SUCCESS;
}

string LeafSetNextPageLogEntryHandler::to_string() const
{
  stringstream ss;
  ss << LogEntryHandler::to_string() << ", new_page_num=" << new_page_num_;
  return ss.str();
}

RC LeafSetNextPageLogEntryHandler::deserialize(Frame *frame, Deserializer &buffer, unique_ptr<LogEntryHandler> &handler)
{
  int     ret      = 0;
  int32_t page_num = -1;
  if ((ret = buffer.read_int32(page_num)) < 0) {
    return RC::INTERNAL;
  }

  handler = make_unique<LeafSetNextPageLogEntryHandler>(frame, page_num, -1 /*old_page_num*/);
  return RC::SUCCESS;
}

RC LeafSetNextPageLogEntryHandler::rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler)
{
  if (nullptr == frame()) {
    return RC::INTERNAL;
  }
  LeafIndexNodeHandler leaf_handler(mtr, tree_handler.file_header(), frame());
  leaf_handler.set_next_page(old_page_num_);
  return RC::SUCCESS;
}

RC LeafSetNextPageLogEntryHandler::redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler)
{
  LeafIndexNodeHandler leaf_handler(mtr, tree_handler.file_header(), frame());

  leaf_handler.set_next_page(new_page_num_);
  return RC::SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// InternalInitEmptyLogEntryHandler
InternalInitEmptyLogEntryHandler::InternalInitEmptyLogEntryHandler(Frame *frame)
    : NodeLogEntryHandler(LogOperation::Type::INTERNAL_INIT_EMPTY, frame)
{}

RC InternalInitEmptyLogEntryHandler::redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler)
{
  InternalIndexNodeHandler internal_handler(mtr, tree_handler.file_header(), frame());
  return internal_handler.init_empty();
}

RC InternalInitEmptyLogEntryHandler::deserialize(
    Frame *frame, Deserializer &buffer, unique_ptr<LogEntryHandler> &handler)
{
  handler = make_unique<InternalInitEmptyLogEntryHandler>(frame);
  return RC::SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// InternalCreateNewRootLogEntryHandler
InternalCreateNewRootLogEntryHandler::InternalCreateNewRootLogEntryHandler(
    Frame *frame, PageNum first_page_num, span<const char> key, PageNum page_num)
    : NodeLogEntryHandler(LogOperation::Type::INTERNAL_CREATE_NEW_ROOT, frame),
      first_page_num_(first_page_num),
      page_num_(page_num),
      key_(key.begin(), key.end())
{}

RC InternalCreateNewRootLogEntryHandler::serialize_body(Serializer &buffer) const
{
  buffer.write_int32(first_page_num_);
  buffer.write_int32(page_num_);
  buffer.write_int32(static_cast<int32_t>(key_.size()));
  buffer.write(key_);
  return RC::SUCCESS;
}

string InternalCreateNewRootLogEntryHandler::to_string() const
{
  stringstream ss;
  ss << LogEntryHandler::to_string() << ", first_page_num=" << first_page_num_ << ", page_num=" << page_num_;
  return ss.str();
}

RC InternalCreateNewRootLogEntryHandler::deserialize(
    Frame *frame, Deserializer &buffer, unique_ptr<LogEntryHandler> &handler)
{
  int ret = 0;

  int32_t first_page_num = -1;
  int32_t page_num       = -1;
  int32_t key_size       = -1;
  if ((ret = buffer.read_int32(first_page_num)) < 0 || (ret = buffer.read_int32(page_num)) < 0 ||
      (ret = buffer.read_int32(key_size)) < 0) {
    return RC::INTERNAL;
  }

  vector<char> key(key_size);
  if ((ret = buffer.read(key)) < 0) {
    return RC::INTERNAL;
  }

  handler = make_unique<InternalCreateNewRootLogEntryHandler>(frame, first_page_num, key, page_num);
  return RC::SUCCESS;
}

RC InternalCreateNewRootLogEntryHandler::redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler)
{
  InternalIndexNodeHandler internal_handler(mtr, tree_handler.file_header(), frame());
  RC rc = internal_handler.create_new_root(first_page_num_, key_.data(), page_num_);
  return rc;
}

///////////////////////////////////////////////////////////////////////////////
// InternalUpdateKeyLogEntryHandler
InternalUpdateKeyLogEntryHandler::InternalUpdateKeyLogEntryHandler(
    Frame *frame, int index, span<const char> key, span<const char> old_key)
    : NodeLogEntryHandler(LogOperation::Type::INTERNAL_UPDATE_KEY, frame),
      index_(index),
      key_(key.begin(), key.end()),
      old_key_(old_key.begin(), old_key.end())
{}

RC InternalUpdateKeyLogEntryHandler::serialize_body(Serializer &buffer) const
{
  buffer.write_int32(index_);
  buffer.write_int32(static_cast<int32_t>(key_.size()));
  buffer.write(key_);
  return RC::SUCCESS;
}

string InternalUpdateKeyLogEntryHandler::to_string() const
{
  stringstream ss;
  ss << LogEntryHandler::to_string() << ", index=" << index_;
  return ss.str();
}

RC InternalUpdateKeyLogEntryHandler::deserialize(
    Frame *frame, Deserializer &buffer, unique_ptr<LogEntryHandler> &handler)
{
  int ret = 0;

  int32_t index    = -1;
  int32_t key_size = -1;
  if ((ret = buffer.read_int32(index)) < 0 || (ret = buffer.read_int32(key_size)) < 0) {
    return RC::INTERNAL;
  }

  vector<char> key(key_size);
  if ((ret = buffer.read(key)) < 0) {
    return RC::INTERNAL;
  }

  vector<char> old_key(0);
  handler = make_unique<InternalUpdateKeyLogEntryHandler>(frame, index, key, old_key);
  return RC::SUCCESS;
}

RC InternalUpdateKeyLogEntryHandler::rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler)
{
  if (nullptr == frame()) {
    return RC::INTERNAL;
  }
  InternalIndexNodeHandler node_handler(mtr, tree_handler.file_header(), frame());
  node_handler.set_key_at(index_, old_key_.data());
  return RC::SUCCESS;
}

RC InternalUpdateKeyLogEntryHandler::redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler)
{
  InternalIndexNodeHandler node_handler(mtr, tree_handler.file_header(), frame());
  node_handler.set_key_at(index_, key_.data());
  return RC::SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// UpdateRootPageLogEntryHandler

UpdateRootPageLogEntryHandler::UpdateRootPageLogEntryHandler(Frame *frame, PageNum root_page_num, PageNum old_page_num)
    : LogEntryHandler(LogOperation::Type::UPDATE_ROOT_PAGE, frame),
      root_page_num_(root_page_num),
      old_page_num_(old_page_num)
{}

RC UpdateRootPageLogEntryHandler::serialize_body(Serializer &buffer) const
{
  buffer.write_int32(root_page_num_);
  return RC::SUCCESS;
}

string UpdateRootPageLogEntryHandler::to_string() const
{
  stringstream ss;
  ss << LogEntryHandler::to_string() << ", root_page_num=" << root_page_num_;
  return ss.str();
}

RC UpdateRootPageLogEntryHandler::deserialize(Frame *frame, Deserializer &buffer, unique_ptr<LogEntryHandler> &handler)
{
  int     ret           = 0;
  int32_t root_page_num = -1;
  if ((ret = buffer.read_int32(root_page_num)) < 0) {
    return RC::INTERNAL;
  }

  handler = make_unique<UpdateRootPageLogEntryHandler>(frame, root_page_num, -1 /*old_page_num*/);
  return RC::SUCCESS;
}

RC UpdateRootPageLogEntryHandler::rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler)
{
  return tree_handler.recover_update_root_page(mtr, old_page_num_);
}

RC UpdateRootPageLogEntryHandler::redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler)
{
  return tree_handler.recover_update_root_page(mtr, root_page_num_);
}

}  // namespace bplus_tree
