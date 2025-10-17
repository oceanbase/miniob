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

#include "common/types.h"
#include "common/lang/functional.h"
#include "storage/table/table_meta.h"
#include "storage/common/chunk.h"

struct RID;
class Record;
class DiskBufferPool;
class RecordFileHandler;
class RecordScanner;
class ChunkFileScanner;
class ConditionFilter;
class DefaultConditionFilter;
class Index;
class IndexScanner;
class RecordDeleter;
class Trx;
class Db;

/**
 * @brief table engine
 */
class TableEngine
{
public:
  TableEngine(TableMeta *table_meta) : table_meta_(table_meta) {}
  virtual ~TableEngine() = default;

  virtual RC insert_record(Record &record)                                                        = 0;
  virtual RC insert_chunk(const Chunk &chunk)                                                     = 0;
  virtual RC delete_record(const Record &record)                                                  = 0;
  virtual RC insert_record_with_trx(Record &record, Trx *trx)                                     = 0;
  virtual RC delete_record_with_trx(const Record &record, Trx *trx)                               = 0;
  virtual RC update_record_with_trx(const Record &old_record, const Record &new_record, Trx *trx) = 0;
  virtual RC get_record(const RID &rid, Record &record)                                           = 0;

  virtual RC     create_index(Trx *trx, const FieldMeta *field_meta, const char *index_name) = 0;
  virtual RC     get_record_scanner(RecordScanner *&scanner, Trx *trx, ReadWriteMode mode)   = 0;
  virtual RC     get_chunk_scanner(ChunkFileScanner &scanner, Trx *trx, ReadWriteMode mode)  = 0;
  virtual RC     visit_record(const RID &rid, function<bool(Record &)> visitor)              = 0;
  virtual RC     sync()                                                                      = 0;
  virtual Index *find_index(const char *index_name) const                                    = 0;
  virtual Index *find_index_by_field(const char *field_name) const                           = 0;
  virtual RC     open()                                                                      = 0;
  // TODO: remove this function
  virtual RC init() = 0;

protected:
  TableMeta *table_meta_ = nullptr;
};
