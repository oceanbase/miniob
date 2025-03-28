/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "storage/trx/lsm_mvcc_trx.h"

RC LsmMvccTrxKit::init() { return RC::SUCCESS; }

const vector<FieldMeta> *LsmMvccTrxKit::trx_fields() const { return nullptr; }

Trx *LsmMvccTrxKit::create_trx(LogHandler &) { return new LsmMvccTrx(lsm_); }

Trx *LsmMvccTrxKit::create_trx(LogHandler &, int32_t /*trx_id*/) { return nullptr; }

void LsmMvccTrxKit::destroy_trx(Trx *trx) { delete trx; }

void LsmMvccTrxKit::all_trxes(vector<Trx *> &trxes) { return; }

/** oblsm 自身的日志回放是足够的，这里其实是空实现 */
LogReplayer *LsmMvccTrxKit::create_log_replayer(Db &, LogHandler &) { return new LsmMvccTrxLogReplayer; }

////////////////////////////////////////////////////////////////////////////////

RC LsmMvccTrx::insert_record(Table *table, Record &record)
{
   return table->insert_record_with_trx(record, this);
}

RC LsmMvccTrx::delete_record(Table *table, Record &record)
{
  return table->delete_record_with_trx(record, this);
}

RC LsmMvccTrx::update_record(Table *table, Record &old_record, Record &new_record)
{
  return table->update_record_with_trx(old_record, new_record, this);
}
/**
 * 在 index scan 中使用的，需要适配 index scan
 */
RC LsmMvccTrx::visit_record(Table *table, Record &record, ReadWriteMode) { return RC::SUCCESS; }

RC LsmMvccTrx::start_if_need()
{
  if (trx_ != nullptr) {
    return RC::SUCCESS;
  }
  trx_ = lsm_->begin_transaction();
  return RC::SUCCESS;
}

RC LsmMvccTrx::commit()
{
  if (trx_ == nullptr) {
    return RC::SUCCESS;
  }
  return trx_->commit();
}

RC LsmMvccTrx::rollback()
{
  return trx_->rollback();
}

/**
 * 实际没有使用
 */
RC LsmMvccTrx::redo(Db *, const LogEntry &) { return RC::SUCCESS; }
