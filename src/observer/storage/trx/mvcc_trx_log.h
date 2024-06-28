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
// Created by Wangyunlai on 2024/02/28.
//

#pragma once

#include "common/rc.h"
#include "common/types.h"
#include "common/lang/string.h"
#include "common/lang/unordered_map.h"
#include "storage/record/record.h"
#include "storage/clog/log_replayer.h"

class LogHandler;
class Table;
class Db;
class MvccTrxKit;
class MvccTrx;
class LogEntry;

/**
 * @brief 表示各种操作类型
 * @ingroup CLog
 */
class MvccTrxLogOperation final
{
public:
  enum class Type : int32_t
  {
    INSERT_RECORD,  ///< 插入一条记录
    DELETE_RECORD,  ///< 删除一条记录
    COMMIT,         ///< 提交事务
    ROLLBACK        ///< 回滚事务
  };

public:
  MvccTrxLogOperation(Type type) : type_(type) {}
  explicit MvccTrxLogOperation(int32_t type) : type_(static_cast<Type>(type)) {}
  ~MvccTrxLogOperation() = default;

  Type    type() const { return type_; }
  int32_t index() const { return static_cast<int32_t>(type_); }

  string to_string() const;

private:
  Type type_;
};

/**
 * @brief 表示事务日志的头部
 * @ingroup CLog
 */
struct MvccTrxLogHeader
{
  int32_t operation_type;  ///< 操作类型
  int32_t trx_id;          ///< 事务ID

  static const int32_t SIZE;  ///< 头部大小

  string to_string() const;
};

/**
 * @brief 表示事务日志中操作行数据的日志，比如插入和删除
 * @ingroup CLog
 * @details 并不记录具体的行数据。
 */
struct MvccTrxRecordLogEntry
{
  MvccTrxLogHeader header;    ///< 日志头部
  int32_t          table_id;  ///< 表ID
  RID              rid;       ///< 记录ID

  static const int32_t SIZE;  ///< 日志大小

  string to_string() const;
};

/**
 * @brief 事务提交的日志
 * @ingroup CLog
 * @details 并没有事务回滚的日志，日志头就可以包含所有回滚日志需要的数据。
 */
struct MvccTrxCommitLogEntry
{
  MvccTrxLogHeader header;         ///< 日志头部
  int32_t          commit_trx_id;  ///< 提交的事务ID

  static const int32_t SIZE;

  string to_string() const;
};

/**
 * @brief 处理事务日志的辅助类
 * @ingroup CLog
 */
class MvccTrxLogHandler final
{
public:
  MvccTrxLogHandler(LogHandler &log_handler);
  ~MvccTrxLogHandler();

  /**
   * @brief 记录插入一条记录的日志
   */
  RC insert_record(int32_t trx_id, Table *table, const RID &rid);

  /**
   * @brief 记录删除一条记录的日志
   */
  RC delete_record(int32_t trx_id, Table *table, const RID &rid);

  /**
   * @brief 记录提交事务的日志
   * @details 会等待日志落地
   */
  RC commit(int32_t trx_id, int32_t commit_trx_id);

  /**
   * @brief 记录回滚事务的日志
   * @details 不会等待日志落地
   */
  RC rollback(int32_t trx_id);

private:
  LogHandler &log_handler_;
};

/**
 * @brief 事务日志回放器
 * @ingroup CLog
 */
class MvccTrxLogReplayer final : public LogReplayer
{
public:
  MvccTrxLogReplayer(Db &db, MvccTrxKit &trx_kit, LogHandler &log_handler);
  virtual ~MvccTrxLogReplayer() = default;

  //! @copydoc LogReplayer::replay
  RC replay(const LogEntry &entry) override;

  //! @copydoc LogReplayer::on_done
  RC on_done() override;

private:
  Db         &db_;           ///< 所属数据库
  MvccTrxKit &trx_kit_;      ///< 事务管理器
  LogHandler &log_handler_;  ///< 日志处理器

  ///< 事务ID到事务的映射。在重做结束后，如果还有未提交的事务，需要回滚。
  unordered_map<int32_t, MvccTrx *> trx_map_;
};