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
// Created by Wangyunlai on 2023/04/24.
//

#pragma once

#include "common/lang/vector.h"
#include "storage/trx/trx.h"
#include "storage/trx/mvcc_trx_log.h"

class CLogManager;
class LogHandler;
class MvccTrxLogHandler;

class MvccTrxKit : public TrxKit
{
public:
  MvccTrxKit() = default;
  virtual ~MvccTrxKit();

  RC                       init() override;
  const vector<FieldMeta> *trx_fields() const override;

  Trx *create_trx(LogHandler &log_handler) override;
  Trx *create_trx(LogHandler &log_handler, int32_t trx_id) override;
  void destroy_trx(Trx *trx) override;

  /**
   * @brief 找到对应事务号的事务
   * @details 当前仅在recover场景下使用
   */
  Trx *find_trx(int32_t trx_id) override;
  void all_trxes(vector<Trx *> &trxes) override;

  LogReplayer *create_log_replayer(Db &db, LogHandler &log_handler) override;

public:
  int32_t next_trx_id();

public:
  int32_t max_trx_id() const;

private:
  vector<FieldMeta> fields_;  // 存储事务数据需要用到的字段元数据，所有表结构都需要带的

  atomic<int32_t> current_trx_id_{0};

  common::Mutex lock_;
  vector<Trx *> trxes_;
};

/**
 * @brief 多版本并发事务
 * @ingroup Transaction
 * TODO 没有垃圾回收
 */
class MvccTrx : public Trx
{
public:
  /**
   * @brief 构造函数
   * @note 外部不应该直接调用该构造函数，而应该使用TrxKit::create_trx()来创建事务，
   * 创建事务时，TrxKit会有一些内部信息需要记录
   */
  MvccTrx(MvccTrxKit &trx_kit, LogHandler &log_handler);
  MvccTrx(MvccTrxKit &trx_kit, LogHandler &log_handler, int32_t trx_id);  // used for recover
  virtual ~MvccTrx();

  RC insert_record(Table *table, Record &record) override;
  RC delete_record(Table *table, Record &record) override;

  /**
   * @brief 当访问到某条数据时，使用此函数来判断是否可见，或者是否有访问冲突
   *
   * @param table    要访问的数据属于哪张表
   * @param record   要访问哪条数据
   * @param mode     是否只读访问
   * @return RC      - SUCCESS 成功
   *                 - RECORD_INVISIBLE 此数据对当前事务不可见，应该跳过
   *                 - LOCKED_CONCURRENCY_CONFLICT 与其它事务有冲突
   */
  RC visit_record(Table *table, Record &record, ReadWriteMode mode) override;

  RC start_if_need() override;
  RC commit() override;
  RC rollback() override;

  RC redo(Db *db, const LogEntry &log_entry) override;

  int32_t id() const override { return trx_id_; }

private:
  RC   commit_with_trx_id(int32_t commit_id);
  void trx_fields(Table *table, Field &begin_xid_field, Field &end_xid_field) const;

private:
  static const int32_t MAX_TRX_ID = numeric_limits<int32_t>::max();

private:
  // using OperationSet = unordered_set<Operation, OperationHasher, OperationEqualer>;
  using OperationSet = vector<Operation>;

  MvccTrxKit       &trx_kit_;
  MvccTrxLogHandler log_handler_;
  int32_t           trx_id_     = -1;
  bool              started_    = false;
  bool              recovering_ = false;
  OperationSet      operations_;
};
