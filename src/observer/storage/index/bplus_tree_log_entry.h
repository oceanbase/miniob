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
class Serializer;
class Deserializer;
}  // namespace common

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

    MAX_TYPE,
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

class LogEntryHandler
{
public:
  LogEntryHandler(LogOperation operation, Frame *frame = nullptr);
  virtual ~LogEntryHandler() = default;

  Frame       *frame() { return frame_; }
  const Frame *frame() const { return frame_; }

  LogOperation operation_type() const { return operation_type_; }

  RC serialize(common::Serializer &buffer) const;
  RC serialize_header(common::Serializer &buffer) const;

  virtual RC serialize_body(common::Serializer &buffer) const                   = 0;
  virtual RC rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) = 0;
  virtual RC redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler)     = 0;

  virtual std::string to_string() const;

  /**
   * @brief 从buffer中反序列化出一个LogEntryHandler
   *
   * @param frame_getter
   * 为了方便测试增加的一个接口。如果把BufferPool抽象的更好，或者把BplusTree日志的二进制数据与Handler解耦，也不需要这样的接口
   * @param buffer 二进制Buffer
   * @param[out] handler 返回结果
   */
  static RC from_buffer(std::function<RC(PageNum, Frame *&)> frame_getter, common::Deserializer &buffer,
      std::unique_ptr<LogEntryHandler> &handler);
  static RC from_buffer(
      DiskBufferPool &buffer_pool, common::Deserializer &buffer, std::unique_ptr<LogEntryHandler> &handler);
  static RC from_buffer(common::Deserializer &deserializer, std::unique_ptr<LogEntryHandler> &handler);

protected:
  LogOperation operation_type_;
  Frame       *frame_ = nullptr;
};

class NodeLogEntryHandler : public LogEntryHandler
{
public:
  NodeLogEntryHandler(LogOperation operation, Frame *frame) : LogEntryHandler(operation, frame) {}

  virtual ~NodeLogEntryHandler() = default;
};

class InitHeaderPageLogEntryHandler : public LogEntryHandler
{
public:
  InitHeaderPageLogEntryHandler(Frame *frame, const IndexFileHeader &file_header);
  virtual ~InitHeaderPageLogEntryHandler() = default;

  RC serialize_body(common::Serializer &buffer) const override;
  RC rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;
  RC redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;

  std::string to_string() const override;

  static RC deserialize(Frame *frame, common::Deserializer &buffer, std::unique_ptr<LogEntryHandler> &handler);

  const IndexFileHeader &file_header() const { return file_header_; }

private:
  IndexFileHeader file_header_;
};

class UpdateRootPageLogEntryHandler : public LogEntryHandler
{
public:
  UpdateRootPageLogEntryHandler(Frame *frame, PageNum root_page_num, PageNum old_page_num);
  virtual ~UpdateRootPageLogEntryHandler() = default;

  RC serialize_body(common::Serializer &buffer) const override;
  RC rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;
  RC redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;

  std::string to_string() const override;

  static RC deserialize(Frame *frame, common::Deserializer &buffer, std::unique_ptr<LogEntryHandler> &handler);

  PageNum root_page_num() const { return root_page_num_; }
  
private:
  PageNum root_page_num_ = -1;
  PageNum old_page_num_  = -1;
};

class SetParentPageLogEntryHandler : public NodeLogEntryHandler
{
public:
  SetParentPageLogEntryHandler(Frame *frame, PageNum parent_page_num, PageNum old_parent_page_num);
  virtual ~SetParentPageLogEntryHandler() = default;

  RC serialize_body(common::Serializer &buffer) const override;
  RC rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;
  RC redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;

  std::string to_string() const override;

  static RC deserialize(Frame *frame, common::Deserializer &buffer, std::unique_ptr<LogEntryHandler> &handler);

  PageNum parent_page_num() const { return parent_page_num_; }

private:
  PageNum parent_page_num_     = -1;
  PageNum old_parent_page_num_ = -1;
};

class NormalOperationLogEntryHandler : public NodeLogEntryHandler
{
public:
  NormalOperationLogEntryHandler(Frame *frame, LogOperation operation, int index, span<const char> items, int item_num);
  virtual ~NormalOperationLogEntryHandler() = default;

  RC serialize_body(common::Serializer &buffer) const override;
  RC rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;
  RC redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;

  std::string to_string() const override;

  static RC deserialize(Frame *frame, LogOperation operation, common::Deserializer &buffer,
      std::unique_ptr<LogEntryHandler> &handler);

  int index() const { return index_; }
  int item_num() const { return item_num_; }
  const char *items() const { return items_.data(); }
  int32_t item_bytes() const { return static_cast<int32_t>(items_.size()); }

private:
  int               index_    = -1;
  int               item_num_ = -1;
  std::vector<char> items_;
};

class LeafInitEmptyLogEntryHandler : public NodeLogEntryHandler
{
public:
  LeafInitEmptyLogEntryHandler(Frame *frame);
  virtual ~LeafInitEmptyLogEntryHandler() = default;

  RC serialize_body(common::Serializer &buffer) const override { return RC::SUCCESS; }
  RC rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override { return RC::SUCCESS; }
  RC redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;

  // std::string to_string() const override;

  static RC deserialize(Frame *frame, common::Deserializer &buffer, std::unique_ptr<LogEntryHandler> &handler);
};

class LeafSetNextPageLogEntryHandler : public NodeLogEntryHandler
{
public:
  LeafSetNextPageLogEntryHandler(Frame *frame, PageNum new_page_num, PageNum old_page_num);
  virtual ~LeafSetNextPageLogEntryHandler() = default;

  RC serialize_body(common::Serializer &buffer) const override;
  RC rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;
  RC redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;

  std::string to_string() const override;

  static RC deserialize(Frame *frame, common::Deserializer &buffer, std::unique_ptr<LogEntryHandler> &handler);

  PageNum new_page_num() const { return new_page_num_; }

private:
  PageNum new_page_num_ = -1;
  PageNum old_page_num_ = -1;
};

class InternalInitEmptyLogEntryHandler : public NodeLogEntryHandler
{
public:
  InternalInitEmptyLogEntryHandler(Frame *frame);
  virtual ~InternalInitEmptyLogEntryHandler() = default;

  RC serialize_body(common::Serializer &buffer) const override { return RC::SUCCESS; }
  RC rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override { return RC::SUCCESS; }
  RC redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;

  // std::string to_string() const override;

  static RC deserialize(Frame *frame, common::Deserializer &buffer, std::unique_ptr<LogEntryHandler> &handler);
};

class InternalCreateNewRootLogEntryHandler : public NodeLogEntryHandler
{
public:
  InternalCreateNewRootLogEntryHandler(
      Frame *frame, PageNum first_page_num, std::span<const char> key, PageNum page_num);
  virtual ~InternalCreateNewRootLogEntryHandler() = default;

  RC serialize_body(common::Serializer &buffer) const override;
  RC rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override { return RC::SUCCESS; }
  RC redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;

  std::string to_string() const override;

  static RC deserialize(Frame *frame, common::Deserializer &buffer, std::unique_ptr<LogEntryHandler> &handler);

  PageNum first_page_num() const { return first_page_num_; }
  PageNum page_num() const { return page_num_; }
  const char *key() const { return key_.data(); }
  int32_t key_bytes() const { return static_cast<int32_t>(key_.size()); }

private:
  PageNum           first_page_num_ = -1;
  PageNum           page_num_       = -1;
  std::vector<char> key_;
};

class InternalUpdateKeyLogEntryHandler : public NodeLogEntryHandler
{
public:
  InternalUpdateKeyLogEntryHandler(Frame *frame, int index, std::span<const char> key, std::span<const char> old_key);
  virtual ~InternalUpdateKeyLogEntryHandler() = default;

  RC serialize_body(common::Serializer &buffer) const override;
  RC rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;
  RC redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;

  std::string to_string() const override;

  static RC deserialize(Frame *frame, common::Deserializer &buffer, std::unique_ptr<LogEntryHandler> &handler);

  int index() const { return index_; }
  const char *key() const { return key_.data(); }
  int32_t key_bytes() const { return static_cast<int32_t>(key_.size()); }
  
private:
  int               index_ = -1;
  std::vector<char> key_;
  std::vector<char> old_key_;
};

}  // namespace bplus_tree
