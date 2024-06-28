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

#pragma once

#include <stddef.h>

#include "common/types.h"
#include "common/rc.h"
#include "common/lang/span.h"
#include "common/lang/memory.h"
#include "common/lang/vector.h"
#include "common/lang/string.h"
#include "storage/index/latch_memo.h"
#include "storage/clog/log_replayer.h"
// #include "storage/index/bplus_tree_log_entry.h"

struct IndexFileHeader;
class LogEntry;
class LogHandler;
class BplusTreeHandler;
class Frame;
class IndexNodeHandler;
class BplusTreeMiniTransaction;
class BufferPoolManager;

namespace bplus_tree {
class LogEntryHandler;
}

namespace common {
class Deserializer;
}

/**
 * @brief B+树日志记录辅助类，同时可以利用此类做回滚操作
 * @ingroup CLog
 * @details 封装B+树持久化需要的各种日志，并负责落地。
 * 如果一个操作中途失败，可以使用rollback进行回滚。
 *
 * 一个B+树的操作，比如插入或删除一个节点，可能由于页面满了而需要分裂，或者由于页面太空了而需要合并，进而
 * 产生很多个日志记录。我们把这些日志记录都放在一个事务中，并生成一个系统中的“大”日志，使这一系列操作要么
 * 都成功，或者一起回滚。
 * 同时，B+树在实际执行中，边执行一些动作修改内存中的数据，边记录日志也记录在内存中。当所有操作执行完成后，
 * 将日志一次性的写入到日志文件中。同时能够保证，修改过的脏页在刷新到磁盘之前，日志已经写入到日志文件中。
 *
 * 在日志回滚或者重做的过程中，其实不需要再记录新的日志，但是调用的B+树本身的接口都是同一个，并没有编写特殊的
 * 类似redo_xxx函数，因此需要这个类本身做判断，是否需要记录日志。
 */
class BplusTreeLogger final
{
public:
  /**
   * @brief 构造函数
   * @param log_handler 日志处理器。实际上就会调用此对象进行日志记录
   * @param buffer_pool_id 关联的缓冲池ID。一个B+树仅记录在一个文件中。
   */
  BplusTreeLogger(LogHandler &log_handler, int32_t buffer_pool_id);
  ~BplusTreeLogger();

  /**
   * @brief 初始化B+树文件头页
   * @details 头页中包含了一些B+树的元信息
   */
  RC init_header_page(Frame *frame, const IndexFileHeader &header);
  /**
   * @brief 更新B+树文件头页，也就是指向根页的页面编号
   * @param root_page_num 更新后的根页编号
   * @param old_page_num 更新前的根页编号。用于回滚
   */
  RC update_root_page(Frame *frame, PageNum root_page_num, PageNum old_page_num);

  /**
   * @brief 在某个页面中插入一些元素
   * @param node_handler 页面处理器。同时也包含了页面编号、页帧
   * @param index 插入的位置
   * @param items 插入的元素
   * @param item_num 元素个数
   */
  RC node_insert_items(IndexNodeHandler &node_handler, int index, span<const char> items, int item_num);
  /**
   * @brief 在某个页面中删除一些元素
   * @param node_handler 页面处理器。同时也包含了页面编号、页帧
   * @param index 插入的位置
   * @param items 插入的元素
   * @param item_num 元素个数
   * @details 会在内存中记录一些数据帮助回滚操作
   */
  RC node_remove_items(IndexNodeHandler &node_handler, int index, span<const char> items, int item_num);

  /**
   * @brief 初始化一个空的叶子节点
   */
  RC leaf_init_empty(IndexNodeHandler &node_handler);
  /**
   * @brief 修改叶子节点的下一个兄弟节点编号
   */
  RC leaf_set_next_page(IndexNodeHandler &node_handler, PageNum page_num, PageNum old_page_num);

  /**
   * @brief 初始化一个空的内部节点
   */
  RC internal_init_empty(IndexNodeHandler &node_handler);
  /**
   * @brief 创建一个新的根节点
   */
  RC internal_create_new_root(
      IndexNodeHandler &node_handler, PageNum first_page_num, span<const char> key, PageNum page_num);
  /**
   * @brief 更新某个内部页面上，更新指定位置的键值
   */
  RC internal_update_key(IndexNodeHandler &node_handler, int index, span<const char> key, span<const char> old_key);

  /**
   * @brief 修改某个页面的父节点编号
   */
  RC set_parent_page(IndexNodeHandler &node_handler, PageNum page_num, PageNum old_page_num);

  /**
   * @brief 提交。表示整个操作成功
   */
  RC commit();
  /**
   * @brief 回滚。操作执行一半失败了，把所有操作都回滚回来
   */
  RC rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler);

  /**
   * @brief 重做日志。通常在系统启动时，会把所有日志重做一遍
   */
  static RC redo(BufferPoolManager &bpm, const LogEntry &entry);
  /**
   * @brief 日志记录转字符串
   */
  static string log_entry_to_string(const LogEntry &entry);

private:
  RC __redo(LSN lsn, BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler, common::Deserializer &redo_buffer);

protected:
  RC append_log_entry(unique_ptr<bplus_tree::LogEntryHandler> entry);

private:
  LogHandler &log_handler_;
  int32_t     buffer_pool_id_ = -1;  /// 关联的缓冲池ID

  vector<unique_ptr<bplus_tree::LogEntryHandler>> entries_;  /// 当前记录了的日志

  bool need_log_ = true;  /// 是否需要记录日志。在回滚或重做过程中，不需要记录日志。
};

/**
 * @brief B+树使用的事务辅助类
 * @ingroup BPlusTree
 * @details B+树的修改操作，通常会涉及到多个动作，比如插入一条数据可能会引起页面分裂，删除一条数据可能会引起页面合并。
 * 我们需要保证这些动作一起成功或一起失败，即使在重启后也保证B+树的一致性。
 */
class BplusTreeMiniTransaction final
{
public:
  /**
   * @brief 构造函数
   * @param tree_handler B+树处理器
   * @param operation_result 操作结果。如果不为nullptr，会在事务结束后，自动根据结果来提交或回滚。
   */
  BplusTreeMiniTransaction(BplusTreeHandler &tree_handler, RC *operation_result = nullptr);
  ~BplusTreeMiniTransaction();

  LatchMemo       &latch_memo() { return latch_memo_; }
  BplusTreeLogger &logger() { return logger_; }

  RC commit();
  RC rollback();

private:
  BplusTreeHandler &tree_handler_;
  RC               *operation_result_ = nullptr;
  LatchMemo         latch_memo_;
  BplusTreeLogger   logger_;
};

/**
 * @brief B+树日志重做器
 * @ingroup CLog
 */
class BplusTreeLogReplayer final : public LogReplayer
{
public:
  BplusTreeLogReplayer(BufferPoolManager &bpm);
  virtual ~BplusTreeLogReplayer() = default;

  /// @copydoc LogReplayer::replay
  virtual RC replay(const LogEntry &entry) override;

private:
  BufferPoolManager &buffer_pool_manager_;
};
