/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
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

#include <stddef.h>
#include <unordered_map>
#include <unordered_set>
#include <mutex>

#include "sql/parser/parse.h"
#include "storage/record/record_manager.h"
#include "storage/common/field_meta.h"
#include "rc.h"

class Table;

class Operation 
{
public:
  enum class Type : int {
    INSERT,
    UPDATE,
    DELETE,
    UNDEFINED,
  };

public:
  Operation(Type type, const RID &rid) : type_(type), page_num_(rid.page_num), slot_num_(rid.slot_num)
  {}

  Type type() const
  {
    return type_;
  }
  PageNum page_num() const
  {
    return page_num_;
  }
  SlotNum slot_num() const
  {
    return slot_num_;
  }

private:
  Type type_;
  // TODO table id
  PageNum page_num_;
  SlotNum slot_num_;
};

class OperationHasher 
{
public:
  size_t operator()(const Operation &op) const
  {
    return (((size_t)op.page_num()) << 32) | (op.slot_num());
  }
};

class OperationEqualer 
{
public:
  bool operator()(const Operation &op1, const Operation &op2) const
  {
    return op1.page_num() == op2.page_num() && op1.slot_num() == op2.slot_num();
  }
};

class TrxKit
{
public:
  enum Type
  {
    VACUOUS,
    MVCC,
  };

public:
  TrxKit() = default;
  virtual ~TrxKit() = default;

  virtual RC init() = 0;
  virtual const TableMeta *trx_fields() const = 0;
  virtual Trx *create_trx() = 0;

public:
  static std::unique_ptr<TrxKit> create(Type type);
};

class VacuousTrxKit : public TrxKit
{
public:
  VacuousTrxKit() = default;
  virtual ~VacuousTrxKit() = default;

  RC init() override;
  const TableMeta *trx_fields() const override;
  Trx *create_trx() override;
};

class MvccTrxKit : public TrxKit
{
public:
  MvccTrxKit() = default;
  virtual ~MvccTrxKit() = default;

  RC init() override;
  const TableMeta *trx_fields() const override;
  Trx *create_trx() override; // TODO unique_ptr?

public:
  int32_t max_trx_id() const;
public:
  TableMeta table_meta_; // 存储事务数据需要用到的字段元数据，所有表结构都需要带的
};

class Trx
{
public:
  Trx() = default;
  virtual ~Trx() = default;
};

class VacuousTrx : public Trx
{
public:
  VacuousTrx() = default;
  virtual ~VacuousTrx() = default;
};

class MvccTrx : public Trx
{
public:
  MvccTrx(MvccTrxKit &trx_kit);
  virtual ~MvccTrx() = default;

  void init_record_insert(Table *table, Record &record) override;
  void mark_record_delete(Table *table, Record &record) override;

private:
  MvccTrxKit &trx_kit_;
};

#if 0
/**
 * 这里是一个简单的事务实现，可以支持提交/回滚。但是没有对并发访问做控制
 * 可以在这个基础上做备份恢复，当然也可以重写
 * Trx: Transaction
 */
class Trx 
{
public:
  static std::atomic<int32_t> trx_id;

  static int32_t default_trx_id();
  static int32_t next_trx_id();
  static void set_trx_id(int32_t id);

  static const char *trx_field_name();
  static AttrType trx_field_type();
  static int trx_field_len();

  static TableMeta record_field_meta;

public:
  Trx() = default;
  virtual ~Trx() = default;

public:
  RC insert_record(Table *table, Record *record);
  RC delete_record(Table *table, Record *record);

  RC commit();
  RC rollback();

  RC commit_insert(Table *table, Record &record);
  RC rollback_delete(Table *table, Record &record);

  bool is_visible(Table *table, const Record *record);

  void init_trx_info(Table *table, Record &record);
  void init_insert_record(Table *table, Record &record);

  void next_current_id();

  int32_t get_current_id();

private:
  void set_record_trx_id(Table *table, Record &record, int32_t trx_id, bool deleted) const;
  static void get_record_trx_id(Table *table, const Record &record, int32_t &trx_id, bool &deleted);

private:
  using OperationSet = std::unordered_set<Operation, OperationHasher, OperationEqualer>;

  Operation *find_operation(Table *table, const RID &rid);
  void insert_operation(Table *table, Operation::Type type, const RID &rid);
  void delete_operation(Table *table, const RID &rid);

private:
  void start_if_not_started();

private:
  int32_t trx_id_ = 0;
  std::unordered_map<Table *, OperationSet> operations_;
};
#endif
