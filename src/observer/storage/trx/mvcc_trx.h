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

#include <vector>

#include "storage/trx/trx.h"

class CLogManager;

class MvccTrxKit : public TrxKit
{
public:
  MvccTrxKit() = default;
  virtual ~MvccTrxKit();

  RC init() override;
  const std::vector<FieldMeta> *trx_fields() const override;
  Trx *create_trx(CLogManager *log_manager) override;
  Trx *create_trx(int32_t trx_id) override;
  void destroy_trx(Trx *trx) override;

  /**
   * @brief 找到对应事务号的事务
   * @details 当前仅在recover场景下使用
   */
  Trx *find_trx(int32_t trx_id) override;
  void all_trxes(std::vector<Trx *> &trxes) override;

public:
  int32_t next_trx_id();

public:
  int32_t max_trx_id() const;

private:
  std::vector<FieldMeta> fields_; // 存储事务数据需要用到的字段元数据，所有表结构都需要带的

  std::atomic<int32_t> current_trx_id_{0};

  common::Mutex      lock_;
  std::vector<Trx *> trxes_;
};

/**
 * @brief 多版本并发事务
 * @ingroup Transaction
 * TODO 没有垃圾回收
 */
class MvccTrx : public Trx
{
public:
  MvccTrx(MvccTrxKit &trx_kit, CLogManager *log_manager);
  MvccTrx(MvccTrxKit &trx_kit, int32_t trx_id); // used for recover
  virtual ~MvccTrx();

  RC insert_record(Table *table, Record &record) override;
  RC delete_record(Table *table, Record &record) override;

  /**
   * @brief 当访问到某条数据时，使用此函数来判断是否可见，或者是否有访问冲突
   * 
   * @param table    要访问的数据属于哪张表
   * @param record   要访问哪条数据
   * @param readonly 是否只读访问
   * @return RC      - SUCCESS 成功
   *                 - RECORD_INVISIBLE 此数据对当前事务不可见，应该跳过
   *                 - LOCKED_CONCURRENCY_CONFLICT 与其它事务有冲突
   */
  RC visit_record(Table *table, Record &record, bool readonly) override;

  RC start_if_need() override;
  RC commit() override;
  RC rollback() override;

  RC redo(Db *db, const CLogRecord &log_record) override;

  int32_t id() const override { return trx_id_; }

private:
  RC commit_with_trx_id(int32_t commit_id);
  void trx_fields(Table *table, Field &begin_xid_field, Field &end_xid_field) const;

private:
  static const int32_t MAX_TRX_ID = std::numeric_limits<int32_t>::max();

private:
  using OperationSet = std::unordered_set<Operation, OperationHasher, OperationEqualer>;
  MvccTrxKit & trx_kit_;
  CLogManager *log_manager_ = nullptr;
  int32_t      trx_id_ = -1;
  bool         started_ = false;
  bool         recovering_ = false;
  OperationSet operations_;
};
