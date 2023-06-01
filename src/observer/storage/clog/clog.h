/* Copyright (c) 2021-2022 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by huhaosheng.hhs on 2022
//

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <list>
#include <atomic>
#include <unordered_map>
#include <deque>
#include <memory>
#include <string>

#include "storage/record/record.h"
#include "storage/persist/persist.h"
#include "common/lang/mutex.h"
#include "rc.h"

class CLogManager;
class CLogBuffer;
class CLogFile;
class Db;

struct CLogRecordHeader;

/**
 * @defgroup CLog
 * @file clog.h
 * @brief CLog 就是 commit log
 * @details 这个模块想要实现数据库事务中的D(durability)，也就是持久化。
 * 持久化是事务四大特性(ACID)中最复杂的模块，这里的实现简化了99.999%，仅在一些特定场景下才能
 * 恢复数据库。
 */

/**
 * @enum CLogType
 * @ingroup CLog
 * @brief 定义clog的几种类型
 * @details 除了事务操作相关的类型，比如MTR_BEGIN/MTR_COMMIT等，都是需要事务自己去处理的。
 * 也就是说，像INSERT、DELETE等是事务自己处理的，其实这种类型的日志不需要在这里定义，而是在各个
 * 事务模型中定义，由各个事务模型自行处理。
 */
#define DEFINE_CLOG_TYPE_ENUM         \
  DEFINE_CLOG_TYPE(ERROR)             \
  DEFINE_CLOG_TYPE(MTR_BEGIN)         \
  DEFINE_CLOG_TYPE(MTR_COMMIT)        \
  DEFINE_CLOG_TYPE(MTR_ROLLBACK)      \
  DEFINE_CLOG_TYPE(INSERT)            \
  DEFINE_CLOG_TYPE(DELETE)

enum class CLogType 
{ 
#define DEFINE_CLOG_TYPE(name) name,
  DEFINE_CLOG_TYPE_ENUM
#undef DEFINE_CLOG_TYPE
};

const char *clog_type_name(CLogType type);
int32_t clog_type_to_integer(CLogType type);
CLogType clog_type_from_integer(int32_t value);

struct CLogRecordHeader 
{
  int32_t lsn_ = -1;     /// log sequence number
  int32_t trx_id_ = -1;
  int32_t type_ = clog_type_to_integer(CLogType::ERROR);
  int32_t logrec_len_ = 0;  /// record的长度，不包含header长度

  bool operator==(const CLogRecordHeader &other) const
  {
    return lsn_ == other.lsn_ && trx_id_ == other.trx_id_ && type_ == other.type_ && logrec_len_ == other.logrec_len_;
  }

  std::string to_string() const;
};

struct CLogRecordData
{
  int32_t          table_id_ = -1;
  RID              rid_;
  int32_t          data_len_ = 0;
  int32_t          data_offset_ = 0;
  char *           data_ = nullptr;

  bool operator==(const CLogRecordData &other) const
  {
    return table_id_ == other.table_id_ &&
      rid_ == other.rid_ &&
      data_len_ == other.data_len_ &&
      data_offset_ == other.data_offset_ &&
      0 == memcmp(data_, other.data_, data_len_);
  }

  std::string to_string() const;

  const static int32_t HEADER_SIZE;
};

class CLogRecord 
{
public:
  CLogRecord() = default;
  ~CLogRecord();

  static CLogRecord *build_mtr_record(CLogType type, int32_t trx_id);
  static CLogRecord *build_data_record(CLogType type, int32_t trx_id, int32_t table_id, const RID &rid, int32_t data_len, int32_t data_offset, const char *data);
  static CLogRecord *build(const CLogRecordHeader &header, char *data);

  CLogType log_type() const  { return clog_type_from_integer(header_.type_); }
  int32_t  trx_id() const { return header_.trx_id_; }
  int32_t  logrec_len() const { return header_.logrec_len_; }

  CLogRecordHeader &header() { return header_; }
  CLogRecordData   &data_record() { return data_record_; }

  const CLogRecordHeader &header() const { return header_; }
  const CLogRecordData   &data_record() const { return data_record_; }

  std::string to_string() const;

protected:
  CLogRecordHeader header_;
  CLogRecordData   data_record_;
};

class CLogBuffer 
{
public:
  CLogBuffer();
  ~CLogBuffer();

  RC append_log_record(CLogRecord *log_record);
  RC flush_buffer(CLogFile &log_file);

private:
  RC write_log_record(CLogFile &log_file, CLogRecord *log_record);

private:
  common::Mutex lock_;
  std::deque<std::unique_ptr<CLogRecord>> log_records_;
  int32_t total_size_;
};

//
#define CLOG_FILE_HDR_SIZE (sizeof(CLogFileHeader))
#define CLOG_BLOCK_SIZE (1 << 9)
#define CLOG_BLOCK_DATA_SIZE (CLOG_BLOCK_SIZE - sizeof(CLogBlockHeader))
#define CLOG_BLOCK_HDR_SIZE (sizeof(CLogBlockHeader))
#define CLOG_REDO_BUFFER_SIZE 8 * CLOG_BLOCK_SIZE

class CLogFile 
{
public:
  CLogFile() = default;
  ~CLogFile();

  RC init(const char *path);
  RC write(const char *data, int len);
  RC read(char *data, int len);
  RC sync();

  bool eof() const { return eof_; }

protected:
  std::string filename_;
  int fd_ = -1;
  bool eof_ = false;
};

class CLogRecordIterator
{
public:
  CLogRecordIterator() = default;
  ~CLogRecordIterator() = default;

  RC init(CLogFile &log_file);

  bool valid() const;
  RC next();
  const CLogRecord &log_record();

private:
  CLogFile *log_file_ = nullptr;
  CLogRecord *log_record_ = nullptr;
};

//
class CLogManager 
{
public:
  CLogManager() = default;
  ~CLogManager();

  RC init(const char *path);

  RC append_log(CLogType type,
                int32_t trx_id,
                int32_t table_id,
                const RID &rid,
                int32_t data_len,
                int32_t data_offset,
                const char *data);

  RC begin_trx(int32_t trx_id);
  RC commit_trx(int32_t trx_id);
  RC rollback_trx(int32_t trx_id);

  RC append_log(CLogRecord *log_record);

  RC sync();

  RC recover(Db *db);

private:
  CLogBuffer *log_buffer_ = nullptr;
  CLogFile *  log_file_   = nullptr;
};
