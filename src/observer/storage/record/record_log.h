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
// Created by Wangyunlai on 2024/02/02.
//

#pragma once

#include <stdint.h>

#include "common/types.h"
#include "common/rc.h"
#include "common/lang/span.h"
#include "common/lang/string.h"
#include "storage/clog/log_replayer.h"
#include "sql/parser/parse_defs.h"

struct RID;
class LogHandler;
class Frame;
class BufferPoolManager;
class DiskBufferPool;

/**
 * @brief 记录管理器操作相关的日志类型
 * @ingroup CLog
 */
class RecordOperation
{
public:
  enum class Type : int32_t
  {
    INIT_PAGE,  /// 初始化空页面
    INSERT,     /// 插入一条记录
    DELETE,     /// 删除一条记录
    UPDATE      /// 更新一条记录
  };

public:
  explicit RecordOperation(Type type) : type_(type) {}
  explicit RecordOperation(int32_t type) : type_(static_cast<Type>(type)) {}
  ~RecordOperation() = default;

  Type    type() const { return type_; }
  int32_t type_id() const { return static_cast<int32_t>(type_); }

  string to_string() const;

private:
  Type type_;
};

struct RecordLogHeader
{
  int32_t buffer_pool_id;
  int32_t operation_type;
  PageNum page_num;
  int32_t storage_format;
  int32_t column_num;
  union
  {
    SlotNum slot_num;
    int32_t record_size;
  };

  char data[0];

  string to_string() const;

  static const int32_t SIZE;
};

class RecordLogHandler final
{
public:
  RecordLogHandler()  = default;
  ~RecordLogHandler() = default;

  RC init(LogHandler &log_handler, int32_t buffer_pool_id, int32_t record_size, StorageFormat storage_format);

  /**
   * @brief 初始化一个新的页面
   * @details 记录一个初始化新页面的日志。
   * @note 这条日志通常伴随着一个buffer pool中创建页面的日志，这时候其实存在一个问题：
   * 通常情况下日志是这样的：
   * 1. buffer pool.allocate page
   * 2. record_log_handler.init_new_page
   * 如果第一条日志记录下来了，但是第二条日志没有记录下来，就会出现问题。就丢失了一个页面，
   * 或者页面在访问时会出现异常。
   * @param frame 页帧
   * @param page_num 页面编号
   * @param data 页面数据目前主要是 `column index`
   */
  RC init_new_page(Frame *frame, PageNum page_num, span<const char> data);

  /**
   * @brief 插入一条记录
   * @param frame 页帧
   * @param rid 记录的位置
   * @param record 记录的内容
   */
  RC insert_record(Frame *frame, const RID &rid, const char *record);

  /**
   * @brief 删除一条记录
   * @param frame 页帧
   * @param rid 记录的位置
   */
  RC delete_record(Frame *frame, const RID &rid);

  /**
   * @brief 更新一条记录
   * @param frame 页帧
   * @param rid 记录的位置
   * @param record 更新后的记录。不需要做回滚，所以不用记录原先的数据
   * @details 更新数据时，通常只更新其中几个字段，这里记录所有数据，是可以优化的。
   */
  RC update_record(Frame *frame, const RID &rid, const char *record);

private:
  LogHandler   *log_handler_    = nullptr;
  int32_t       buffer_pool_id_ = -1;
  int32_t       record_size_    = -1;
  StorageFormat storage_format_ = StorageFormat::ROW_FORMAT;
};

/**
 * @brief 记录相关的日志重放器
 * @ingroup CLog
 */
class RecordLogReplayer final : public LogReplayer
{
public:
  RecordLogReplayer(BufferPoolManager &bpm);
  virtual ~RecordLogReplayer() = default;

  virtual RC replay(const LogEntry &entry) override;

private:
  RC replay_init_page(DiskBufferPool &buffer_pool, const RecordLogHeader &log_header);
  RC replay_insert(DiskBufferPool &buffer_pool, const RecordLogHeader &log_header);
  RC replay_delete(DiskBufferPool &buffer_pool, const RecordLogHeader &log_header);
  RC replay_update(DiskBufferPool &buffer_pool, const RecordLogHeader &log_header);

private:
  BufferPoolManager &bpm_;
};
