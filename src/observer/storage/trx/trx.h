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
// Created by Wangyunlai on 2021/5/24.
//

#pragma once

#include <mutex>
#include <stddef.h>
#include <unordered_set>
#include <utility>

#include "common/rc.h"
#include "sql/parser/parse.h"
#include "storage/field/field_meta.h"
#include "storage/record/record_manager.h"
#include "storage/table/table.h"

/**
 * @defgroup Transaction
 * @brief 事务相关的内容
 */

class Db;
class CLogManager;
class CLogRecord;
class Trx;

/**
 * @brief 描述一个操作，比如插入、删除行等
 * @ingroup Transaction
 * @details 通常包含一个操作的类型，以及操作的对象和具体的数据
 */
class Operation
{
public:
  /**
   * @brief 操作的类型
   * @ingroup Transaction
   */
  enum class Type : int
  {
    INSERT,
    UPDATE,
    DELETE,
    UNDEFINED,
  };

public:
  Operation(Type type, Table *table, const RID &rid)
      : type_(type), table_(table), page_num_(rid.page_num), slot_num_(rid.slot_num)
  {}

  Type    type() const { return type_; }
  int32_t table_id() const { return table_->table_id(); }
  Table  *table() const { return table_; }
  PageNum page_num() const { return page_num_; }
  SlotNum slot_num() const { return slot_num_; }

private:
  ///< 操作的哪张表。这里直接使用表其实并不准确，因为表中的索引也可能有日志
  Type type_;

  Table  *table_ = nullptr;
  PageNum page_num_;  // TODO use RID instead of page num and slot num
  SlotNum slot_num_;
};

class OperationHasher
{
public:
  size_t operator()(const Operation &op) const { return (((size_t)op.page_num()) << 32) | (op.slot_num()); }
};

class OperationEqualer
{
public:
  bool operator()(const Operation &op1, const Operation &op2) const
  {
    return op1.table_id() == op2.table_id() && op1.page_num() == op2.page_num() && op1.slot_num() == op2.slot_num();
  }
};

/**
 * @brief 事务管理器
 * @ingroup Transaction
 */
class TrxKit
{
public:
  /**
   * @brief 事务管理器的类型
   * @ingroup Transaction
   * @details 进程启动时根据事务管理器的类型来创建具体的对象
   */
  enum Type
  {
    VACUOUS,  ///< 空的事务管理器，不做任何事情
    MVCC,     ///< 支持MVCC的事务管理器
  };

public:
  TrxKit()          = default;
  virtual ~TrxKit() = default;

  virtual RC                            init()                               = 0;
  virtual const std::vector<FieldMeta> *trx_fields() const                   = 0;
  virtual Trx                          *create_trx(CLogManager *log_manager) = 0;
  virtual Trx                          *create_trx(int32_t trx_id)           = 0;
  virtual Trx                          *find_trx(int32_t trx_id)             = 0;
  virtual void                          all_trxes(std::vector<Trx *> &trxes) = 0;

  virtual void destroy_trx(Trx *trx) = 0;

public:
  static TrxKit *create(const char *name);
  static RC      init_global(const char *name);
  static TrxKit *instance();
};

/**
 * @brief 事务接口
 * @ingroup Transaction
 */
class Trx
{
public:
  Trx()          = default;
  virtual ~Trx() = default;

  virtual RC insert_record(Table *table, Record &record)               = 0;
  virtual RC delete_record(Table *table, Record &record)               = 0;
  virtual RC visit_record(Table *table, Record &record, bool readonly) = 0;

  virtual RC start_if_need() = 0;
  virtual RC commit()        = 0;
  virtual RC rollback()      = 0;

  virtual RC redo(Db *db, const CLogRecord &log_record);

  virtual int32_t id() const = 0;
};
