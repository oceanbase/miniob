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

#include "common/types.h"
#include "common/rc.h"
#include "common/lang/span.h"
#include "common/lang/string.h"
#include "storage/index/bplus_tree.h"

class IndexNodeHandler;
class BplusTreeHandler;
class BplusTreeMiniTransaction;

namespace common {
class Serializer;
class Deserializer;
}  // namespace common

namespace bplus_tree {

/**
 * @brief B+树日志操作类型
 * @ingroup CLog
 */
class LogOperation
{
public:
  enum class Type
  {
    INIT_HEADER_PAGE,          /// 初始化B+树文件头
    UPDATE_ROOT_PAGE,          /// 更新根节点
    SET_PARENT_PAGE,           /// 设置父节点
    LEAF_INIT_EMPTY,           /// 初始化叶子节点
    LEAF_SET_NEXT_PAGE,        /// 设置叶子节点的兄弟节点
    INTERNAL_INIT_EMPTY,       /// 初始化内部节点
    INTERNAL_CREATE_NEW_ROOT,  /// 创建新的根节点
    INTERNAL_UPDATE_KEY,       /// 更新内部节点的key
    NODE_INSERT,               /// 在节点中间(也可能是末尾)插入一些元素
    NODE_REMOVE,               /// 在节点中间(也可能是末尾)删除一些元素

    MAX_TYPE,
  };

public:
  LogOperation(Type type) : type_(type) {}
  explicit LogOperation(int type) : type_(static_cast<Type>(type)) {}

  Type type() const { return type_; }
  int  index() const { return static_cast<int>(type_); }

  string to_string() const;

private:
  Type type_;
};

/**
 * @brief B+树日志处理辅助类
 * @ingroup CLog
 * @details 每种操作类型的日志，都有一个具体的实现类。
 */
class LogEntryHandler
{
public:
  LogEntryHandler(LogOperation operation, Frame *frame = nullptr);
  virtual ~LogEntryHandler() = default;

  /**
   * @brief 返回日志对应的frame
   * @details 每条日志都对应着操作关联的页面。但是在日志重放时，或者只是想将日志内容格式化时，是没有对应的页面的。
   */
  Frame       *frame() { return frame_; }
  const Frame *frame() const { return frame_; }

  PageNum page_num() const;
  void    set_page_num(PageNum page_num) { page_num_ = page_num; }

  /// @brief 日志操作类型
  LogOperation operation_type() const { return operation_type_; }

  /// @brief 序列化日志
  RC serialize(common::Serializer &buffer) const;
  /// @brief 序列化日志头
  RC serialize_header(common::Serializer &buffer) const;

  /// @brief 序列化日志内容。所有子类应该实现这个函数
  virtual RC serialize_body(common::Serializer &buffer) const = 0;

  /**
   * @brief 回滚
   * @details 在事务回滚时，需要根据日志内容进行回滚
   */
  virtual RC rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) = 0;

  /**
   * @brief 重做
   * @details 在系统重启时，需要根据日志内容进行重做
   */
  virtual RC redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) = 0;

  virtual string to_string() const;

  /**
   * @brief 从buffer中反序列化出一个LogEntryHandler
   *
   * @param frame_getter
   * 为了方便测试增加的一个接口。如果把BufferPool抽象的更好，或者把BplusTree日志的二进制数据与Handler解耦，也不需要这样的接口
   * @param buffer 二进制Buffer
   * @param[out] handler 返回结果
   * @details
   * 这里有个比较别扭的地方。一个日志应该有两种表现形式，一个是内存或磁盘中的二进制数据，另一个是可以进行操作的Handler。
   * 但是这里实现的时候，只使用了handler，所以在反序列化时，有些场景下根本不需要Frame，但是为了适配，允许传入一个null
   * frame。 在LogEntryHandler类中也做了特殊处理。就是虽然有frame指针对象，但是也另外单独记录了page_num。
   */
  static RC from_buffer(
      function<RC(PageNum, Frame *&)> frame_getter, common::Deserializer &buffer, unique_ptr<LogEntryHandler> &handler);
  static RC from_buffer(
      DiskBufferPool &buffer_pool, common::Deserializer &buffer, unique_ptr<LogEntryHandler> &handler);
  static RC from_buffer(common::Deserializer &deserializer, unique_ptr<LogEntryHandler> &handler);

protected:
  LogOperation operation_type_;
  Frame       *frame_ = nullptr;

  /// page num本来存放在frame中。但是只有在运行时才能拿到frame，为了强制适配
  /// 解析文件buffer时不存在运行时的情况，直接记录page num
  PageNum page_num_ = BP_INVALID_PAGE_NUM;
};

/**
 * @brief 节点相关的操作
 * @ingroup CLog
 */
class NodeLogEntryHandler : public LogEntryHandler
{
public:
  NodeLogEntryHandler(LogOperation operation, Frame *frame) : LogEntryHandler(operation, frame) {}

  virtual ~NodeLogEntryHandler() = default;
};

/**
 * @brief 初始化B+树文件头日志处理类
 * @ingroup CLog
 */
class InitHeaderPageLogEntryHandler : public LogEntryHandler
{
public:
  InitHeaderPageLogEntryHandler(Frame *frame, const IndexFileHeader &file_header);
  virtual ~InitHeaderPageLogEntryHandler() = default;

  RC serialize_body(common::Serializer &buffer) const override;
  RC rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;
  RC redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;

  string to_string() const override;

  static RC deserialize(Frame *frame, common::Deserializer &buffer, unique_ptr<LogEntryHandler> &handler);

  const IndexFileHeader &file_header() const { return file_header_; }

private:
  IndexFileHeader file_header_;
};

/**
 * @brief 更新根节点日志处理类
 * @ingroup CLog
 */
class UpdateRootPageLogEntryHandler : public LogEntryHandler
{
public:
  UpdateRootPageLogEntryHandler(Frame *frame, PageNum root_page_num, PageNum old_page_num);
  virtual ~UpdateRootPageLogEntryHandler() = default;

  RC serialize_body(common::Serializer &buffer) const override;
  RC rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;
  RC redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;

  string to_string() const override;

  static RC deserialize(Frame *frame, common::Deserializer &buffer, unique_ptr<LogEntryHandler> &handler);

  PageNum root_page_num() const { return root_page_num_; }

private:
  PageNum root_page_num_ = -1;
  PageNum old_page_num_  = -1;
};

/**
 * @brief 设置父节点日志处理类
 * @ingroup CLog
 */
class SetParentPageLogEntryHandler : public NodeLogEntryHandler
{
public:
  SetParentPageLogEntryHandler(Frame *frame, PageNum parent_page_num, PageNum old_parent_page_num);
  virtual ~SetParentPageLogEntryHandler() = default;

  RC serialize_body(common::Serializer &buffer) const override;
  RC rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;
  RC redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;

  string to_string() const override;

  static RC deserialize(Frame *frame, common::Deserializer &buffer, unique_ptr<LogEntryHandler> &handler);

  PageNum parent_page_num() const { return parent_page_num_; }

private:
  PageNum parent_page_num_     = -1;
  PageNum old_parent_page_num_ = -1;
};

/**
 * @brief 插入或者删除一些节点上的元素
 * @ingroup CLog
 */
class NormalOperationLogEntryHandler : public NodeLogEntryHandler
{
public:
  NormalOperationLogEntryHandler(Frame *frame, LogOperation operation, int index, span<const char> items, int item_num);
  virtual ~NormalOperationLogEntryHandler() = default;

  RC serialize_body(common::Serializer &buffer) const override;
  RC rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;
  RC redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;

  string to_string() const override;

  static RC deserialize(
      Frame *frame, LogOperation operation, common::Deserializer &buffer, unique_ptr<LogEntryHandler> &handler);

  int         index() const { return index_; }
  int         item_num() const { return item_num_; }
  const char *items() const { return items_.data(); }
  int32_t     item_bytes() const { return static_cast<int32_t>(items_.size()); }

private:
  int          index_    = -1;
  int          item_num_ = -1;
  vector<char> items_;
};

/**
 * @brief 叶子节点初始化日志处理类
 * @ingroup CLog
 */
class LeafInitEmptyLogEntryHandler : public NodeLogEntryHandler
{
public:
  LeafInitEmptyLogEntryHandler(Frame *frame);
  virtual ~LeafInitEmptyLogEntryHandler() = default;

  RC serialize_body(common::Serializer &buffer) const override { return RC::SUCCESS; }
  RC rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override { return RC::SUCCESS; }
  RC redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;

  // string to_string() const override;

  static RC deserialize(Frame *frame, common::Deserializer &buffer, unique_ptr<LogEntryHandler> &handler);
};

/**
 * @brief 设置叶子节点的兄弟节点日志处理类
 * @ingroup CLog
 */
class LeafSetNextPageLogEntryHandler : public NodeLogEntryHandler
{
public:
  LeafSetNextPageLogEntryHandler(Frame *frame, PageNum new_page_num, PageNum old_page_num);
  virtual ~LeafSetNextPageLogEntryHandler() = default;

  RC serialize_body(common::Serializer &buffer) const override;
  RC rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;
  RC redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;

  string to_string() const override;

  static RC deserialize(Frame *frame, common::Deserializer &buffer, unique_ptr<LogEntryHandler> &handler);

  PageNum new_page_num() const { return new_page_num_; }

private:
  PageNum new_page_num_ = -1;
  PageNum old_page_num_ = -1;
};

/**
 * @brief 初始化内部节点日志处理类
 * @ingroup CLog
 */
class InternalInitEmptyLogEntryHandler : public NodeLogEntryHandler
{
public:
  InternalInitEmptyLogEntryHandler(Frame *frame);
  virtual ~InternalInitEmptyLogEntryHandler() = default;

  RC serialize_body(common::Serializer &buffer) const override { return RC::SUCCESS; }
  RC rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override { return RC::SUCCESS; }
  RC redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;

  // string to_string() const override;

  static RC deserialize(Frame *frame, common::Deserializer &buffer, unique_ptr<LogEntryHandler> &handler);
};

/**
 * @brief 创建新的根节点日志处理类
 * @ingroup CLog
 */
class InternalCreateNewRootLogEntryHandler : public NodeLogEntryHandler
{
public:
  InternalCreateNewRootLogEntryHandler(Frame *frame, PageNum first_page_num, span<const char> key, PageNum page_num);
  virtual ~InternalCreateNewRootLogEntryHandler() = default;

  RC serialize_body(common::Serializer &buffer) const override;
  RC rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override { return RC::SUCCESS; }
  RC redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;

  string to_string() const override;

  static RC deserialize(Frame *frame, common::Deserializer &buffer, unique_ptr<LogEntryHandler> &handler);

  PageNum     first_page_num() const { return first_page_num_; }
  PageNum     page_num() const { return page_num_; }
  const char *key() const { return key_.data(); }
  int32_t     key_bytes() const { return static_cast<int32_t>(key_.size()); }

private:
  PageNum      first_page_num_ = -1;
  PageNum      page_num_       = -1;
  vector<char> key_;
};

/**
 * @brief 更新内部节点的key日志处理类
 * @ingroup CLog
 */
class InternalUpdateKeyLogEntryHandler : public NodeLogEntryHandler
{
public:
  InternalUpdateKeyLogEntryHandler(Frame *frame, int index, span<const char> key, span<const char> old_key);
  virtual ~InternalUpdateKeyLogEntryHandler() = default;

  RC serialize_body(common::Serializer &buffer) const override;
  RC rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;
  RC redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;

  string to_string() const override;

  static RC deserialize(Frame *frame, common::Deserializer &buffer, unique_ptr<LogEntryHandler> &handler);

  int         index() const { return index_; }
  const char *key() const { return key_.data(); }
  int32_t     key_bytes() const { return static_cast<int32_t>(key_.size()); }

private:
  int          index_ = -1;
  vector<char> key_;
  vector<char> old_key_;
};

}  // namespace bplus_tree
