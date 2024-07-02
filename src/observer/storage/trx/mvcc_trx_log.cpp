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

#include "storage/trx/mvcc_trx_log.h"
#include "storage/trx/mvcc_trx.h"
#include "storage/table/table.h"
#include "storage/clog/log_module.h"
#include "storage/clog/log_handler.h"
#include "storage/clog/log_entry.h"
#include "storage/db/db.h"
#include "storage/table/table.h"

using namespace common;

string MvccTrxLogOperation::to_string() const
{
  string ret = std::to_string(index()) + ":";
  switch (type_) {
    case Type::INSERT_RECORD: return ret + "INSERT_RECORD";
    case Type::DELETE_RECORD: return ret + "DELETE_RECORD";
    case Type::COMMIT: return ret + "COMMIT";
    case Type::ROLLBACK: return ret + "ROLLBACK";
    default: return ret + "UNKNOWN";
  }
}

const int32_t MvccTrxLogHeader::SIZE = sizeof(MvccTrxLogHeader);

string MvccTrxLogHeader::to_string() const
{
  stringstream ss;
  ss << "operation_type:" << MvccTrxLogOperation(operation_type).to_string() << ", trx_id:" << trx_id;
  return ss.str();
}

const int32_t MvccTrxRecordLogEntry::SIZE = sizeof(MvccTrxRecordLogEntry);

string MvccTrxRecordLogEntry::to_string() const
{
  stringstream ss;
  ss << header.to_string() << ", table_id: " << table_id << ", rid: " << rid.to_string();
  return ss.str();
}

const int32_t MvccTrxCommitLogEntry::SIZE = sizeof(MvccTrxCommitLogEntry);

string MvccTrxCommitLogEntry::to_string() const
{
  stringstream ss;
  ss << header.to_string() << ", commit_trx_id: " << commit_trx_id;
  return ss.str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

MvccTrxLogHandler::MvccTrxLogHandler(LogHandler &log_handler) : log_handler_(log_handler) {}

MvccTrxLogHandler::~MvccTrxLogHandler() {}

RC MvccTrxLogHandler::insert_record(int32_t trx_id, Table *table, const RID &rid)
{
  ASSERT(trx_id > 0, "invalid trx_id:%d", trx_id);

  MvccTrxRecordLogEntry log_entry;
  log_entry.header.operation_type = MvccTrxLogOperation(MvccTrxLogOperation::Type::INSERT_RECORD).index();
  log_entry.header.trx_id         = trx_id;
  log_entry.table_id              = table->table_id();
  log_entry.rid                   = rid;

  LSN lsn = 0;
  return log_handler_.append(
      lsn, LogModule::Id::TRANSACTION, span<const char>(reinterpret_cast<const char *>(&log_entry), sizeof(log_entry)));
}

RC MvccTrxLogHandler::delete_record(int32_t trx_id, Table *table, const RID &rid)
{
  ASSERT(trx_id > 0, "invalid trx_id:%d", trx_id);

  MvccTrxRecordLogEntry log_entry;
  log_entry.header.operation_type = MvccTrxLogOperation(MvccTrxLogOperation::Type::DELETE_RECORD).index();
  log_entry.header.trx_id         = trx_id;
  log_entry.table_id              = table->table_id();
  log_entry.rid                   = rid;

  LSN lsn = 0;
  return log_handler_.append(
      lsn, LogModule::Id::TRANSACTION, span<const char>(reinterpret_cast<const char *>(&log_entry), sizeof(log_entry)));
}

RC MvccTrxLogHandler::commit(int32_t trx_id, int32_t commit_trx_id)
{
  ASSERT(trx_id > 0 && commit_trx_id > trx_id, "invalid trx_id:%d, commit_trx_id:%d", trx_id, commit_trx_id);

  MvccTrxCommitLogEntry log_entry;
  log_entry.header.operation_type = MvccTrxLogOperation(MvccTrxLogOperation::Type::COMMIT).index();
  log_entry.header.trx_id         = trx_id;
  log_entry.commit_trx_id         = commit_trx_id;

  LSN lsn = 0;
  RC rc = log_handler_.append(
      lsn, LogModule::Id::TRANSACTION, span<const char>(reinterpret_cast<const char *>(&log_entry), sizeof(log_entry)));
  if (OB_FAIL(rc)) {
    return rc;
  }

  // 我们在这里粗暴的等待日志写入到磁盘
  // 有必要的话，可以让上层来决定如何等待
  return log_handler_.wait_lsn(lsn);
}

RC MvccTrxLogHandler::rollback(int32_t trx_id)
{
  ASSERT(trx_id > 0, "invalid trx_id:%d", trx_id);

  MvccTrxCommitLogEntry log_entry;
  log_entry.header.operation_type = MvccTrxLogOperation(MvccTrxLogOperation::Type::ROLLBACK).index();
  log_entry.header.trx_id         = trx_id;

  LSN lsn = 0;
  return log_handler_.append(
      lsn, LogModule::Id::TRANSACTION, span<const char>(reinterpret_cast<const char *>(&log_entry), sizeof(log_entry)));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
MvccTrxLogReplayer::MvccTrxLogReplayer(Db &db, MvccTrxKit &trx_kit, LogHandler &log_handler)
  : db_(db), trx_kit_(trx_kit), log_handler_(log_handler)
{}

RC MvccTrxLogReplayer::replay(const LogEntry &entry)
{
  RC rc = RC::SUCCESS;

  // 由于当前没有check point，所以所有的事务都重做。

  ASSERT(entry.module().id() == LogModule::Id::TRANSACTION, "invalid log module id: %d", entry.module().id());

  if (entry.payload_size() < MvccTrxLogHeader::SIZE) {
    LOG_WARN("invalid log entry size: %d, trx log header size:%ld", entry.payload_size(), MvccTrxLogHeader::SIZE);
    return RC::LOG_ENTRY_INVALID;
  }

  auto *header = reinterpret_cast<const MvccTrxLogHeader *>(entry.data());
  MvccTrx *trx = nullptr;
  auto trx_iter = trx_map_.find(header->trx_id);
  if (trx_iter == trx_map_.end()) {
    trx = static_cast<MvccTrx *>(trx_kit_.create_trx(log_handler_, header->trx_id));
    // trx = new MvccTrx(trx_kit_, log_handler_, header->trx_id);
  } else {
    trx = trx_iter->second;
  }

  /// 直接调用事务代码自己的重放函数
  rc = trx->redo(&db_, entry);

  /// 如果事务结束了，需要从内存中把它删除
  if (MvccTrxLogOperation(header->operation_type).type() == MvccTrxLogOperation::Type::ROLLBACK ||
      MvccTrxLogOperation(header->operation_type).type() == MvccTrxLogOperation::Type::COMMIT) {
    Trx *trx = trx_map_[header->trx_id];
    trx_kit_.destroy_trx(trx);
    trx_map_.erase(header->trx_id);
  }

  return rc;
}

RC MvccTrxLogReplayer::on_done()
{
  /// 日志回放已经完成，需要把没有提交的事务，回滚掉
  for (auto &pair : trx_map_) {
    MvccTrx *trx = pair.second;
    trx->rollback(); // 恢复时的rollback，可能遇到之前已经回滚一半的事务又再次调用回滚的情况
    delete pair.second;
  }
  trx_map_.clear();

  return RC::SUCCESS;
}
