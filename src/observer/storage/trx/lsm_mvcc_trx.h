/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#pragma once

#include "storage/trx/trx.h"
#include "storage/db/db.h"
#include "oblsm/include/ob_lsm.h"
#include "oblsm/include/ob_lsm_transaction.h"

using namespace oceanbase;
/**
 * @brief lsm-tree 存储引擎对应的事务管理器
 */
class LsmMvccTrxKit : public TrxKit
{
public:
  LsmMvccTrxKit(Db *db) : lsm_(db->lsm()) {}
  virtual ~LsmMvccTrxKit() = default;

  RC                       init() override;
  const vector<FieldMeta> *trx_fields() const override;

  Trx *create_trx(LogHandler &log_handler) override;
  Trx *create_trx(LogHandler &log_handler, int32_t trx_id) override;
  void all_trxes(vector<Trx *> &trxes) override;

  void destroy_trx(Trx *trx) override;

  LogReplayer *create_log_replayer(Db &db, LogHandler &log_handler) override;

private:
  oceanbase::ObLsm *lsm_;
};

class LsmMvccTrx : public Trx
{
public:
  LsmMvccTrx(ObLsm *lsm) : Trx(TrxKit::Type::LSM), lsm_(lsm), trx_(nullptr) {}
  virtual ~LsmMvccTrx()
  {
    if (trx_ != nullptr) {
      delete trx_;
    }
  }

  RC insert_record(Table *table, Record &record) override;
  RC delete_record(Table *table, Record &record) override;
  RC update_record(Table *table, Record &old_record, Record &record) override;
  RC visit_record(Table *table, Record &record, ReadWriteMode mode) override;
  RC start_if_need() override;
  RC commit() override;
  RC rollback() override;

  RC redo(Db *db, const LogEntry &log_entry) override;

  ObLsmTransaction *get_trx() { return trx_; }

  int32_t id() const override { return 0; }

private:
  ObLsm            *lsm_;
  ObLsmTransaction *trx_ = nullptr;
};

class LsmMvccTrxLogReplayer : public LogReplayer
{
public:
  LsmMvccTrxLogReplayer()          = default;
  virtual ~LsmMvccTrxLogReplayer() = default;

  RC replay(const LogEntry &) override { return RC::SUCCESS; }
};