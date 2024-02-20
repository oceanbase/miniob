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

#pragma once

#include <string>
#include <span>

#include "common/types.h"
#include "common/rc.h"
#include "storage/index/bplus_tree.h"

class IndexNodeHandler;
class BplusTreeHandler;
class BplusTreeMiniTransaction;

namespace common {
class SerializeBuffer;
}

namespace bplus_tree {

class LogOperation
{
public:
  enum class Type
  {
    INIT_HEADER_PAGE,
    UPDATE_ROOT_PAGE,
    SET_PARENT_PAGE,
    LEAF_INIT_EMPTY,
    LEAF_SET_NEXT_PAGE,
    INTERNAL_INIT_EMPTY,
    INTERNAL_CREATE_NEW_ROOT,
    INTERNAL_UPDATE_KEY,
    NODE_INSERT,  // 在节点中间(也可能是末尾)插入一些元素
    NODE_REMOVE,  // 在节点中间(也可能是末尾)删除一些元素
  };

public:
  LogOperation(Type type) : type_(type) {}
  explicit LogOperation(int type) : type_(static_cast<Type>(type)) {}

  Type type() const { return type_; }
  int  index() const { return static_cast<int>(type_); }

  std::string to_string() const;

private:
  Type type_;
};

class LogEntry
{
public:
  LogEntry(LogOperation operation, Frame *frame) : operation_type_(operation), frame_(frame) {}
  virtual ~LogEntry() = default;

  Frame *frame() { return frame_; }

  LogOperation operation_type() const { return operation_type_; }

  RC serialize(common::SerializeBuffer &buffer) const;
  RC serialize_header(common::SerializeBuffer &buffer) const;

  virtual RC serialize_body(common::SerializeBuffer &buffer) const                   = 0;
  virtual RC rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) = 0;

protected:
  LogOperation operation_type_;
  Frame       *frame_ = nullptr;
};

class NodeLogEntry : public LogEntry
{
public:
  NodeLogEntry(LogOperation operation, IndexNodeHandler &node_handler)
      : LogEntry(operation, node_handler.frame()), node_handler_(node_handler)
  {}

  virtual ~NodeLogEntry() = default;

protected:
  IndexNodeHandler &node_handler_;
};

class InitHeaderPageLogEntry : public LogEntry
{
public:
  InitHeaderPageLogEntry(
      Frame *frame, int32_t attr_length, int32_t attr_type, int32_t internal_max_size, int32_t leaf_max_size)
      : LogEntry(LogOperation::Type::INIT_HEADER_PAGE, frame),
        attr_length_(attr_length),
        attr_type_(attr_type),
        internal_max_size_(internal_max_size),
        leaf_max_size_(leaf_max_size)
  {}
  virtual ~InitHeaderPageLogEntry() = default;

  RC serialize_body(common::SerializeBuffer &buffer) const override;
  RC rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;

private:
  int32_t attr_length_       = -1;
  int32_t attr_type_         = -1;
  int32_t internal_max_size_ = -1;
  int32_t leaf_max_size_     = -1;
};

class UpdateRootPageLogEntry : public LogEntry
{
public:
  UpdateRootPageLogEntry(Frame *frame, PageNum root_page_num, PageNum old_page_num)
      : LogEntry(LogOperation::Type::UPDATE_ROOT_PAGE, frame),
        root_page_num_(root_page_num),
        old_page_num_(old_page_num)
  {}
  virtual ~UpdateRootPageLogEntry() = default;

  RC serialize_body(common::SerializeBuffer &buffer) const override;
  RC rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;

private:
  PageNum root_page_num_ = -1;
  PageNum old_page_num_  = -1;
};

class SetParentPageLogEntry : public NodeLogEntry
{
public:
  SetParentPageLogEntry(IndexNodeHandler &node_handler, PageNum parent_page_num, PageNum old_parent_page_num)
      : NodeLogEntry(LogOperation::Type::SET_PARENT_PAGE, node_handler),
        parent_page_num_(parent_page_num),
        old_parent_page_num_(old_parent_page_num)
  {}
  virtual ~SetParentPageLogEntry() = default;

  RC serialize_body(common::SerializeBuffer &buffer) const override;
  RC rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;

private:
  PageNum parent_page_num_     = -1;
  PageNum old_parent_page_num_ = -1;
};

class NormalOperationLogEntry : public NodeLogEntry
{
public:
  NormalOperationLogEntry(
      IndexNodeHandler &node_handler, LogOperation operation, int index, std::span<const char> items, int item_num)
      : NodeLogEntry(operation, node_handler), index_(index), item_num_(item_num), items_(items.begin(), items.end())
  {}
  virtual ~NormalOperationLogEntry() = default;

  RC serialize_body(common::SerializeBuffer &buffer) const override;
  RC rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;

private:
  int               index_    = -1;
  int               item_num_ = -1;
  std::vector<char> items_;
};

class LeafInitEmptyLogEntry : public NodeLogEntry
{
public:
  LeafInitEmptyLogEntry(IndexNodeHandler &node_handler)
      : NodeLogEntry(LogOperation::Type::LEAF_INIT_EMPTY, node_handler)
  {}
  virtual ~LeafInitEmptyLogEntry() = default;

  RC serialize_body(common::SerializeBuffer &buffer) const override { return RC::SUCCESS; }
  RC rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override { return RC::SUCCESS; }
};

class LeafSetNextPageLogEntry : public NodeLogEntry
{
public:
  LeafSetNextPageLogEntry(IndexNodeHandler &node_handler, PageNum new_page_num, PageNum old_page_num)
      : NodeLogEntry(LogOperation::Type::LEAF_SET_NEXT_PAGE, node_handler),
        new_page_num_(new_page_num),
        old_page_num_(old_page_num)
  {}
  virtual ~LeafSetNextPageLogEntry() = default;

  RC serialize_body(common::SerializeBuffer &buffer) const override;
  RC rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;

private:
  PageNum new_page_num_ = -1;
  PageNum old_page_num_ = -1;
};

class InternalInitEmptyLogEntry : public NodeLogEntry
{
public:
  InternalInitEmptyLogEntry(IndexNodeHandler &node_handler)
      : NodeLogEntry(LogOperation::Type::INTERNAL_INIT_EMPTY, node_handler)
  {}
  virtual ~InternalInitEmptyLogEntry() = default;

  RC serialize_body(common::SerializeBuffer &buffer) const override { return RC::SUCCESS; }
  RC rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override { return RC::SUCCESS; }
};

class InternalCreateNewRootLogEntry : public NodeLogEntry
{
public:
  InternalCreateNewRootLogEntry(IndexNodeHandler &node_handler, PageNum first_page_num, std::span<const char> key,
      PageNum page_num)
      : NodeLogEntry(LogOperation::Type::INTERNAL_CREATE_NEW_ROOT, node_handler),
        first_page_num_(first_page_num),
        page_num_(page_num),
        key_(key.begin(), key.end())
  {}
  virtual ~InternalCreateNewRootLogEntry() = default;

  RC serialize_body(common::SerializeBuffer &buffer) const override;
  RC rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override { return RC::SUCCESS; }

private:
  PageNum           first_page_num_ = -1;
  PageNum           page_num_       = -1;
  std::vector<char> key_;
};

class InternalUpdateKeyLogEntry : public NodeLogEntry
{
public:
  InternalUpdateKeyLogEntry(
      IndexNodeHandler &node_handler, int index, std::span<const char> key, std::span<const char> old_key)
      : NodeLogEntry(LogOperation::Type::INTERNAL_UPDATE_KEY, node_handler),
        key_(key.begin(), key.end()),
        old_key_(old_key.begin(), old_key.end())
  {}
  virtual ~InternalUpdateKeyLogEntry() = default;

  RC serialize_body(common::SerializeBuffer &buffer) const override;
  RC rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;

private:
  int               index_ = -1;
  std::vector<char> key_;
  std::vector<char> old_key_;
};

}  // namespace bplus_tree
