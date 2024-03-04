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

#include <string>

#include "common/rc.h"
#include "common/types.h"
#include "storage/record/record.h"
#include "storage/clog/log_replayer.h"

class LogHandler;
class Table;
class Db;
class MvccTrxKit;
class MvccTrx;
class LogEntry;

class MvccTrxLogOperation final
{
public:
  enum class Type : int32_t
  {
    INSERT_RECORD,
    DELETE_RECORD,
    COMMIT,
    ROLLBACK
  };

public:
  MvccTrxLogOperation(Type type) : type_(type) {}
  explicit MvccTrxLogOperation(int32_t type) : type_(static_cast<Type>(type)) {}
  ~MvccTrxLogOperation() = default;

  Type type() const { return type_; }
  int32_t index() const { return static_cast<int32_t>(type_); }

  std::string to_string() const;

private:
  Type type_;
};

struct MvccTrxLogHeader
{
  int32_t operation_type;
  int32_t trx_id;

  static const int32_t SIZE;

  std::string to_string() const;
};

struct MvccTrxRecordLogEntry
{
  MvccTrxLogHeader header;
  int32_t table_id;
  RID rid;

  static const int32_t SIZE;

  std::string to_string() const;
};

struct MvccTrxCommitLogEntry
{
  MvccTrxLogHeader header;
  int32_t commit_trx_id;

  static const int32_t SIZE;

  std::string to_string() const;
};

class MvccTrxLogHandler final
{
public:
  MvccTrxLogHandler(LogHandler &log_handler);
  ~MvccTrxLogHandler();

  RC insert_record(int32_t trx_id, Table *table, const RID &rid);
  RC delete_record(int32_t trx_id, Table *table, const RID &rid);
  RC commit(int32_t trx_id, int32_t commit_trx_id);
  RC rollback(int32_t trx_id);

private:
  LogHandler &log_handler_;
};

class MvccTrxLogReplayer final : public LogReplayer
{
public:
  MvccTrxLogReplayer(Db &db, MvccTrxKit &trx_kit, LogHandler &log_handler);
  virtual ~MvccTrxLogReplayer() = default;

  RC replay(const LogEntry &entry) override;

  RC on_done() override;

private:
  Db &db_;
  MvccTrxKit &trx_kit_;
  LogHandler &log_handler_;
  std::unordered_map<int32_t, MvccTrx *> trx_map_;
};