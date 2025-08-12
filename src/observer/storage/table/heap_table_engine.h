/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#pragma once

#include "storage/table/table_engine.h"
#include "storage/index/index.h"
#include "storage/record/record_manager.h"
#include "storage/db/db.h"

class Table;
/**
 * @brief table engine
 */
class HeapTableEngine : public TableEngine
{
public:
  friend class Table;
  HeapTableEngine(TableMeta *table_meta, Db *db, Table *table) : TableEngine(table_meta), db_(db), table_(table) {}
  ~HeapTableEngine() override;

  RC insert_record(Record &record) override;
  RC insert_chunk(const Chunk &chunk) override;
  RC delete_record(const Record &record) override;
  RC insert_record_with_trx(Record &record, Trx *trx) override { return RC::UNSUPPORTED; }
  RC delete_record_with_trx(const Record &record, Trx *trx) override { return RC::UNSUPPORTED; }
  RC update_record_with_trx(const Record &old_record, const Record &new_record, Trx *trx) override
  {
    return RC::UNSUPPORTED;
  }
  RC get_record(const RID &rid, Record &record) override;

  RC create_index(Trx *trx, const FieldMeta *field_meta, const char *index_name) override;
  RC get_record_scanner(RecordScanner *&scanner, Trx *trx, ReadWriteMode mode) override;
  RC get_chunk_scanner(ChunkFileScanner &scanner, Trx *trx, ReadWriteMode mode) override;
  RC visit_record(const RID &rid, function<bool(Record &)> visitor) override;
  RC sync() override;

  Index *find_index(const char *index_name) const override;
  Index *find_index_by_field(const char *field_name) const override;
  RC     open() override;
  // init_record_handler
  RC init() override;

private:
  RC insert_entry_of_indexes(const char *record, const RID &rid);
  RC delete_entry_of_indexes(const char *record, const RID &rid, bool error_on_not_exists);

private:
  DiskBufferPool    *data_buffer_pool_ = nullptr;  /// 数据文件关联的buffer pool
  RecordFileHandler *record_handler_   = nullptr;  /// 记录操作
  vector<Index *>    indexes_;
  Db                *db_;
  Table             *table_;
};
