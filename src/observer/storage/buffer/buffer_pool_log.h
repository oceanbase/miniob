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
// Created by wangyunlai on 2022/02/01
//

#pragma once

#include "common/lang/string.h"
#include "common/types.h"
#include "common/rc.h"
#include "storage/clog/log_replayer.h"

class DiskBufferPool;
class BufferPoolManager;
class LogHandler;
struct Page;

/**
 * @brief BufferPool 的日志相关操作类型
 * @ingroup CLog
 */
class BufferPoolOperation
{
public:
  enum class Type : int32_t
  {
    ALLOCATE,   /// 分配页面
    DEALLOCATE  /// 释放页面
  };

public:
  BufferPoolOperation(Type type) : type_(type) {}
  explicit BufferPoolOperation(int32_t type) : type_(static_cast<Type>(type)) {}
  ~BufferPoolOperation() = default;

  Type    type() const { return type_; }
  int32_t type_id() const { return static_cast<int32_t>(type_); }

  string to_string() const
  {
    string ret = std::to_string(type_id()) + ":";
    switch (type_) {
      case Type::ALLOCATE: return ret + "ALLOCATE";
      case Type::DEALLOCATE: return ret + "DEALLOCATE";
      default: return ret + "UNKNOWN";
    }
  }

private:
  Type type_;
};

/**
 * @brief BufferPool 的日志记录
 * @ingroup CLog
 */
struct BufferPoolLogEntry
{
  int32_t buffer_pool_id;  /// buffer pool id
  int32_t operation_type;  /// operation type
  PageNum page_num;        /// page number

  string to_string() const;
};

/**
 * @brief BufferPool 的日志记录处理器
 * @ingroup CLog
 */
class BufferPoolLogHandler final
{
public:
  BufferPoolLogHandler(DiskBufferPool &buffer_pool, LogHandler &log_handler);
  ~BufferPoolLogHandler() = default;

  /**
   * @brief 分配一个页面
   * @param page_num 分配的页面号
   * @param[out] lsn 分配页面的日志序列号
   * @note TODO 可以把frame传过来，记录完日志，直接更新页面的lsn
   */
  RC allocate_page(PageNum page_num, LSN &lsn);

  /**
   * @brief 释放一个页面
   * @param page_num 释放的页面编号
   * @param[out] lsn 释放页面的日志序列号
   */
  RC deallocate_page(PageNum page_num, LSN &lsn);

  /**
   * @brief 刷新页面到磁盘之前，需要保证页面对应的日志也已经刷新到磁盘
   * @details 如果页面刷新到磁盘了，但是日志很落后，在重启恢复时，就会出现异常，无法让所有的页面都恢复到一致的状态。
   */
  RC flush_page(Page &page);

private:
  RC append_log(BufferPoolOperation::Type type, PageNum page_num, LSN &lsn);

private:
  DiskBufferPool &buffer_pool_;
  LogHandler     &log_handler_;
};

/**
 * @brief BufferPool 的日志重放器
 * @ingroup CLog
 */
class BufferPoolLogReplayer final : public LogReplayer
{
public:
  BufferPoolLogReplayer(BufferPoolManager &bp_manager);
  virtual ~BufferPoolLogReplayer() = default;

  ///! @copydoc LogReplayer::replay
  RC replay(const LogEntry &entry) override;

private:
  BufferPoolManager &bp_manager_;
};
