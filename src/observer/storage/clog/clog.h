/* Copyright (c) 2021-2022 Xie Meiyi(xiemeiyi@hust.edu.cn),
Huazhong University of Science and Technology
and OceanBase and/or its affiliates. All rights reserved.
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

#ifndef __OBSERVER_STORAGE_REDO_REDOLOG_H_
#define __OBSERVER_STORAGE_REDO_REDOLOG_H_

#include <stddef.h>
#include <stdint.h>
#include <list>
#include <atomic>
#include <unordered_map>

#include "storage/record/record.h"
#include "storage/persist/persist.h"
#include "rc.h"

//固定文件大小 TODO: 循环文件组
#define CLOG_FILE_SIZE 48 * 1024 * 1024
#define CLOG_BUFFER_SIZE 4 * 1024 * 1024
#define TABLE_NAME_MAX_LEN 20  // TODO: 表名不要超过20字节

class CLogManager;
class CLogBuffer;
class CLogFile;
class PersistHandler;

struct CLogRecordHeader;
struct CLogFileHeader;
struct CLogBlockHeader;
struct CLogBlock;
struct CLogMTRManager;

enum CLogType { REDO_ERROR = 0, REDO_MTR_BEGIN, REDO_MTR_COMMIT, REDO_INSERT, REDO_DELETE };

struct CLogRecordHeader {
  int32_t lsn_;
  int32_t trx_id_;
  int type_;
  int logrec_len_;

  bool operator==(const CLogRecordHeader &other) const
  {
    return lsn_ == other.lsn_ && trx_id_ == other.trx_id_ && type_ == other.type_ && logrec_len_ == other.logrec_len_;
  }
};

struct CLogInsertRecord {
  CLogRecordHeader hdr_;
  char table_name_[TABLE_NAME_MAX_LEN];
  RID rid_;
  int data_len_;
  char *data_;

  bool operator==(const CLogInsertRecord &other) const
  {
    return hdr_ == other.hdr_ && (strcmp(table_name_, other.table_name_) == 0) && (rid_ == other.rid_) &&
           (data_len_ == other.data_len_) && (memcmp(data_, other.data_, data_len_) == 0);
  }
};

struct CLogDeleteRecord {
  CLogRecordHeader hdr_;
  char table_name_[TABLE_NAME_MAX_LEN];
  RID rid_;

  bool operator==(const CLogDeleteRecord &other) const
  {
    return hdr_ == other.hdr_ && strcmp(table_name_, other.table_name_) == 0 && rid_ == other.rid_;
  }
};

struct CLogMTRRecord {
  CLogRecordHeader hdr_;

  bool operator==(const CLogMTRRecord &other) const
  {
    return hdr_ == other.hdr_;
  }
};

union CLogRecords {
  CLogInsertRecord ins;
  CLogDeleteRecord del;
  CLogMTRRecord mtr;
  char *errors;
};

class CLogRecord {
  friend class Db;

public:
  // TODO: lsn当前在内部分配
  // 对齐在内部处理
  CLogRecord(CLogType flag, int32_t trx_id, const char *table_name = nullptr, int data_len = 0, Record *rec = nullptr);
  // 从外存恢复log record
  CLogRecord(char *data);
  ~CLogRecord();

  CLogType get_log_type()
  {
    return flag_;
  }
  int32_t get_trx_id()
  {
    return log_record_.mtr.hdr_.trx_id_;
  }
  int32_t get_logrec_len()
  {
    return log_record_.mtr.hdr_.logrec_len_;
  }
  int32_t get_lsn()
  {
    return log_record_.mtr.hdr_.lsn_;
  }
  RC copy_record(void *dest, int start_off, int copy_len);

  /// for unitest
  int cmp_eq(CLogRecord *other);
  CLogRecords *get_record()
  {
    return &log_record_;
  }
  ///

protected:
  CLogType flag_;
  CLogRecords log_record_;
};

// TODO: 当前为简单实现，无循环
class CLogBuffer {
public:
  CLogBuffer();
  ~CLogBuffer();

  RC append_log_record(CLogRecord *log_rec, int &start_off);
  // 将buffer中的数据下刷到log_file
  RC flush_buffer(CLogFile *log_file);
  void set_current_block_no(const int32_t block_no)
  {
    current_block_no_ = block_no;
  }
  void set_write_block_offset(const int32_t write_block_offset)
  {
    write_block_offset_ = write_block_offset;
  };
  void set_write_offset(const int32_t write_offset)
  {
    write_offset_ = write_offset;
  };

  RC block_copy(int32_t offset, CLogBlock *log_block);

protected:
  int32_t current_block_no_;
  int32_t write_block_offset_;
  int32_t write_offset_;

  char buffer_[CLOG_BUFFER_SIZE];
};

//
#define CLOG_FILE_HDR_SIZE (sizeof(CLogFileHeader))
#define CLOG_BLOCK_SIZE (1 << 9)
#define CLOG_BLOCK_DATA_SIZE (CLOG_BLOCK_SIZE - sizeof(CLogBlockHeader))
#define CLOG_BLOCK_HDR_SIZE (sizeof(CLogBlockHeader))
#define CLOG_REDO_BUFFER_SIZE 8 * CLOG_BLOCK_SIZE

struct CLogRecordBuf {
  int32_t write_offset_;
  // TODO: 当前假定log record大小不会超过CLOG_REDO_BUFFER_SIZE
  char buffer_[CLOG_REDO_BUFFER_SIZE];
};

struct CLogFileHeader {
  int32_t current_file_real_offset_;
  // TODO: 用于文件组，当前没用
  int32_t current_file_lsn_;
};

struct CLogFHDBlock {
  CLogFileHeader hdr_;
  char pad[CLOG_BLOCK_SIZE - CLOG_FILE_HDR_SIZE];
};

struct CLogBlockHeader {
  int32_t log_block_no;  // 在文件中的offset no=n*CLOG_BLOCK_SIZE
  int16_t log_data_len_;
  int16_t first_rec_offset_;
};

struct CLogBlock {
  CLogBlockHeader log_block_hdr_;
  char data[CLOG_BLOCK_DATA_SIZE];
};

class CLogFile {
public:
  CLogFile(const char *path);
  ~CLogFile();

  RC update_log_fhd(int32_t current_file_lsn);
  RC append(int data_len, char *data);
  RC write(uint64_t offset, int data_len, char *data);
  RC recover(CLogMTRManager *mtr_mgr, CLogBuffer *log_buffer);
  RC block_recover(CLogBlock *block, int16_t &offset, CLogRecordBuf *logrec_buf, CLogRecord *&log_rec);

protected:
  CLogFHDBlock log_fhd_;
  PersistHandler *log_file_;
};

// TODO: 当前简单管理mtr
struct CLogMTRManager {
  std::list<CLogRecord *> log_redo_list;
  std::unordered_map<int32_t, bool> trx_commited;  // <trx_id, commited>

  void log_record_manage(CLogRecord *log_rec);
};

//
class CLogManager {
public:
  CLogManager(const char *path);
  ~CLogManager();

  RC init();

  RC clog_gen_record(CLogType flag, int32_t trx_id, CLogRecord *&log_rec, const char *table_name = nullptr,
      int data_len = 0, Record *rec = nullptr);
  //追加写到log_buffer
  RC clog_append_record(CLogRecord *log_rec);
  // 通常不需要在外部调用
  RC clog_sync();
  // TODO: 优化回放过程，对同一位置的修改可以用哈希聚合
  RC recover();

  CLogMTRManager *get_mtr_manager();

  static int32_t get_next_lsn(int32_t rec_len);

  static std::atomic<int32_t> gloabl_lsn_;

protected:
  CLogBuffer *log_buffer_;
  CLogFile *log_file_;
  CLogMTRManager *log_mtr_mgr_;
};

#endif  // __OBSERVER_STORAGE_REDO_REDOLOG_H_
