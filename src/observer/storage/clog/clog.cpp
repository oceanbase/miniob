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

#include <sstream>
#include <vector>

#include "common/log/log.h"
#include "storage/clog/clog.h"
#include "common/global_context.h"
#include "storage/trx/trx.h"
#include "common/io/io.h"

using namespace std;
using namespace common;

/**
 * @brief 当前的日志使用固定的文件名，而且就这一个文件
 */
const char *CLOG_FILE_NAME = "clog";

const char *clog_type_name(CLogType type)
{
  #define DEFINE_CLOG_TYPE(name)  case CLogType::name: return #name;
  switch (type) {
    DEFINE_CLOG_TYPE_ENUM;
    default: return "unknown clog type";
  }
  #undef DEFINE_CLOG_TYPE
}

int32_t clog_type_to_integer(CLogType type)
{
  return static_cast<int32_t>(type);
}
CLogType clog_type_from_integer(int32_t value)
{
  return static_cast<CLogType>(value);
}

////////////////////////////////////////////////////////////////////////////////

string CLogRecordHeader::to_string() const
{
  stringstream ss;
  ss << "lsn:" << lsn_
     << ", trx_id:" << trx_id_
     << ", type:" << clog_type_name(clog_type_from_integer(type_)) << "(" << type_ << ")"
     << ", len:" << logrec_len_;
  return ss.str();
}

////////////////////////////////////////////////////////////////////////////////

string CLogRecordCommitData::to_string() const
{
  stringstream ss;
  ss << "commit_xid:" << commit_xid_;
  return ss.str();
}

////////////////////////////////////////////////////////////////////////////////

const int32_t CLogRecordData::HEADER_SIZE = sizeof(CLogRecordData) - sizeof(CLogRecordData::data_);

CLogRecordData::~CLogRecordData()
{
  if (data_ == nullptr) {
    delete[] data_;
  }
}
string CLogRecordData::to_string() const
{
  stringstream ss;
  ss << "table_id:" << table_id_ << ", rid:{" << rid_.to_string() << "}"
     << ", len:" << data_len_ << ", offset:" << data_offset_;
  return ss.str();
}

////////////////////////////////////////////////////////////////////////////////

int _align8(int size)
{
  return size / 8 * 8 + ((size % 8 == 0) ? 0 : 8);
}

CLogRecord *CLogRecord::build_mtr_record(CLogType type, int32_t trx_id)
{
  CLogRecord *log_record = new CLogRecord();
  CLogRecordHeader &header = log_record->header_;
  header.trx_id_ = trx_id;
  header.type_   = clog_type_to_integer(type);
  return log_record;
}

CLogRecord *CLogRecord::build_commit_record(int32_t trx_id, int32_t commit_xid)
{
  CLogRecord *log_record = new CLogRecord();
  CLogRecordHeader &header = log_record->header_;
  header.type_ = clog_type_to_integer(CLogType::MTR_COMMIT);
  header.trx_id_ = trx_id;
  header.logrec_len_ = sizeof(CLogRecordCommitData);
  
  CLogRecordCommitData &commit_record = log_record->commit_record();
  commit_record.commit_xid_ = commit_xid;
  return log_record;
}

CLogRecord *CLogRecord::build_data_record(CLogType type, 
                                          int32_t trx_id, 
                                          int32_t table_id, 
                                          const RID &rid, 
                                          int32_t data_len, 
                                          int32_t data_offset, 
                                          const char *data)
{
  CLogRecord *log_record = new CLogRecord();
  CLogRecordHeader &header = log_record->header_;
  header.trx_id_ = trx_id;
  header.type_   = clog_type_to_integer(type);
  header.logrec_len_ = CLogRecordData::HEADER_SIZE + data_len;

  CLogRecordData &data_record = log_record->data_record();
  data_record.table_id_    = table_id;
  data_record.rid_         = rid;
  data_record.data_len_    = data_len;
  data_record.data_offset_ = data_offset;

  if (data_len > 0) {
    data_record.data_ = new char[data_len];
    if (nullptr == data_record.data_) {
      delete log_record;
      LOG_WARN("failed to allocate memory while creating clog record. memory size=%d", data_len);
      return nullptr;
    }

    memcpy(data_record.data_, data, data_len);
  }
  return log_record;
}

CLogRecord *CLogRecord::build(const CLogRecordHeader &header, char *data)
{
  CLogRecord *log_record = new CLogRecord();
  log_record->header_    = header;

  if (header.logrec_len_ <= 0) {
    return log_record;
  }

  if (header.type_ == clog_type_to_integer(CLogType::MTR_COMMIT)) {
    ASSERT(header.logrec_len_ == sizeof(CLogRecordCommitData), "invalid length of mtr commit. expect %d, got %d",
           sizeof(CLogRecordCommitData), header.logrec_len_);
    
    CLogRecordCommitData &commit_record = log_record->commit_record();
    memcpy(reinterpret_cast<void *>(&commit_record), data, sizeof(CLogRecordCommitData));

    LOG_DEBUG("got a commit record %s", log_record->to_string().c_str());
  } else {
    /// 当前日志拥有数据，但是不是COMMIT，就认为是普通的修改数据的日志，简单粗暴
    CLogRecordData &data_record = log_record->data_record();
    memcpy(reinterpret_cast<void *>(&data_record), data, CLogRecordData::HEADER_SIZE);
    if (header.logrec_len_ > CLogRecordData::HEADER_SIZE) {
      int data_len      = header.logrec_len_ - CLogRecordData::HEADER_SIZE;
      data_record.data_ = new char[data_len];
      memcpy(data_record.data_, data + CLogRecordData::HEADER_SIZE, data_len);
    }
  }
  return log_record;
}

CLogRecord::~CLogRecord()
{
}

string CLogRecord::to_string() const
{
  if (header_.logrec_len_ <= 0) {
    return header_.to_string();
  } else if (header_.type_ == clog_type_to_integer(CLogType::MTR_COMMIT)) {
    return header_.to_string() + ", " + commit_record().to_string();
  } else {
    return header_.to_string() + ", " + data_record().to_string();
  }
}

////////////////////////////////////////////////////////////////////////////////
static const int CLOG_BUFFER_SIZE = 4 * 1024 * 1024;

CLogBuffer::CLogBuffer()
{
}

CLogBuffer::~CLogBuffer()
{}

RC CLogBuffer::append_log_record(CLogRecord *log_record)
{
  if (nullptr == log_record) {
    return RC::INVALID_ARGUMENT;
  }

  /// total_size_ 的计算没有考虑日志头
  if (total_size_ + log_record->logrec_len() >= CLOG_BUFFER_SIZE) {
    return RC::LOGBUF_FULL;
  }

  lock_guard<Mutex> lock_guard(lock_);
  log_records_.emplace_back(log_record);
  total_size_ += log_record->logrec_len();
  LOG_DEBUG("append log. log_record={%s}", log_record->to_string().c_str());
  return RC::SUCCESS;
}

RC CLogBuffer::flush_buffer(CLogFile &log_file)
{
  RC rc = RC::SUCCESS;
  int count = 0;
  while (!log_records_.empty()) {
    lock_.lock();
    if (log_records_.empty()) {
      lock_.unlock();
      return RC::SUCCESS;
    }

    // log buffer 需要支持并发，所以要考虑加锁
    // 从队列中取出日志记录然后写入到文件中
    unique_ptr<CLogRecord> log_record = std::move(log_records_.front());
    log_records_.pop_front();

    rc = write_log_record(log_file, log_record.get());
    // 当前无法处理日志写不完整的情况，所以直接粗暴退出
    ASSERT(rc == RC::SUCCESS, "failed to write log record. log_record=%s, rc=%s",
           log_record->to_string().c_str(), strrc(rc));

    lock_.unlock();
    total_size_ -= log_record->logrec_len();
    count++;
  }

  LOG_WARN("flush log buffer done. write log record number=%d", count);
  return log_file.sync();
}

RC CLogBuffer::write_log_record(CLogFile &log_file, CLogRecord *log_record)
{
  // TODO 看起来每种类型的日志自己实现 serialize 接口更好一点
  const CLogRecordHeader &header = log_record->header();
  RC rc = log_file.write(reinterpret_cast<const char *>(&header), sizeof(header));
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to write log record header. size=%d, rc=%s", sizeof(header), strrc(rc));
    return rc;
  }

  switch (log_record->log_type()) {
    case CLogType::MTR_BEGIN:
    case CLogType::MTR_ROLLBACK: {
      // do nothing
    } break;

    case CLogType::MTR_COMMIT: {
      rc = log_file.write(reinterpret_cast<const char *>(&log_record->commit_record()), 
                          log_record->header().logrec_len_);
    } break;

    default: {
      rc = log_file.write(reinterpret_cast<const char *>(&log_record->data_record()), CLogRecordData::HEADER_SIZE);
      if (OB_FAIL(rc)) {
        LOG_WARN("failed to write data record header. size=%d, rc=%s", CLogRecordData::HEADER_SIZE, strrc(rc));
        return rc;
      }

      rc = log_file.write(log_record->data_record().data_, log_record->data_record().data_len_);
      if (OB_FAIL(rc)) {
        LOG_WARN("failed to write log data. size=%d, rc=%s", log_record->data_record().data_len_, strrc(rc));
        return rc;
      }
    } break;
  }

  return rc;
}

////////////////////////////////////////////////////////////////////////////////

RC CLogFile::init(const char *path)
{
  RC rc = RC::SUCCESS;

  std::string clog_file_path = std::string(path) + common::FILE_PATH_SPLIT_STR + CLOG_FILE_NAME;
  int fd = ::open(clog_file_path.c_str(), O_RDWR | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);
  if (fd < 0) {
    rc = RC::IOERR_OPEN;
    LOG_WARN("failed to open clog file. filename=%s, error=%s", clog_file_path.c_str(), strerror(errno));
    return rc;
  }

  filename_ = clog_file_path;
  fd_       = fd;
  LOG_INFO("open clog file success. file=%s, fd=%d", filename_.c_str(), fd_);
  return rc;
}

CLogFile::~CLogFile()
{
  if (fd_ >= 0) {
    LOG_INFO("close clog file. file=%s, fd=%d", filename_.c_str(), fd_);
    ::close(fd_);
    fd_ = -1;
  }
}

RC CLogFile::write(const char *data, int len)
{
  int ret = writen(fd_, data, len);
  if (0 != ret) {
    LOG_WARN("failed to write data to file. filename=%s, data len=%d, error=%s", filename_.c_str(), len, strerror(ret));
    return RC::IOERR_WRITE;
  }
  return RC::SUCCESS;
}

RC CLogFile::read(char *data, int len)
{
  int ret = readn(fd_, data, len);
  if (ret != 0) {
    if (ret == -1) {
      eof_ = true;
      LOG_TRACE("file read touch eof. filename=%s", filename_.c_str());
    } else {
      LOG_WARN("failed to read data from file. file=%s, data len=%d, error=%s", filename_.c_str(), len, strerror(ret));
    }
    return RC::IOERR_READ;
  }
  return RC::SUCCESS;
}

RC CLogFile::sync()
{
  int ret = fsync(fd_);
  if (ret != 0) {
    LOG_WARN("failed to sync file. file=%s, error=%s", filename_.c_str(), strerror(errno));
    return RC::IOERR_SYNC;
  }
  return RC::SUCCESS;
}

RC CLogFile::offset(int64_t &off) const
{
  off_t pos = lseek(fd_, 0, SEEK_CUR);
  if (pos == -1) {
    LOG_WARN("failed to seek. error=%s", strerror(errno));
    return RC::IOERR_SEEK;
  }

  off = static_cast<int64_t>(pos);
  return RC::SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
RC CLogRecordIterator::init(CLogFile &log_file)
{
  log_file_ = &log_file;
  return RC::SUCCESS;
}

bool CLogRecordIterator::valid() const
{
  return nullptr != log_record_;
}

RC CLogRecordIterator::next()
{
  delete log_record_;
  log_record_ = nullptr;

  CLogRecordHeader header;
  RC rc = log_file_->read(reinterpret_cast<char *>(&header), sizeof(header));
  if (rc != RC::SUCCESS) {
    if (log_file_->eof()) {
      return RC::RECORD_EOF;
    }

    LOG_WARN("failed to read log header. rc=%s", strrc(rc));
    return rc;
  }

  char *data = nullptr;
  int32_t record_size = header.logrec_len_;
  if (record_size > 0) {
    data = new char[record_size];
    rc = log_file_->read(data, record_size);
    if (OB_FAIL(rc)) {
      if (log_file_->eof()) {
        // TODO 遇到了没有写完整数据的log，应该truncate一部分数据, 但是现在不管
      }
      LOG_WARN("failed to read log data. data size=%d, rc=%s", record_size, strrc(rc));
      delete[] data;
      data = nullptr;
      return rc;
    }
  }

  delete log_record_;
  log_record_ = CLogRecord::build(header, data);
  delete[] data;
  return rc;
}

const CLogRecord &CLogRecordIterator::log_record()
{
  return *log_record_;
}

////////////////////////////////////////////////////////////////////////////////

RC CLogManager::init(const char *path)
{
  log_buffer_ = new CLogBuffer();
  log_file_   = new CLogFile();
  return log_file_->init(path);
}

CLogManager::~CLogManager()
{
  if (log_buffer_) {
    delete log_buffer_;
    log_buffer_ = nullptr;
  }

  if (log_file_ != nullptr) {
    delete log_file_;
    log_file_ = nullptr;
  }
}

RC CLogManager::append_log(CLogType type, 
                int32_t trx_id, 
                int32_t table_id, 
                const RID &rid, 
                int32_t data_len, 
                int32_t data_offset, 
                const char *data)
{
  CLogRecord *log_record = CLogRecord::build_data_record(type, trx_id, table_id, rid, data_len, data_offset, data);
  if (nullptr == log_record) {
    LOG_WARN("failed to create log record");
    return RC::NOMEM;
  }
  return append_log(log_record);
}

RC CLogManager::begin_trx(int32_t trx_id)
{
  return append_log(CLogRecord::build_mtr_record(CLogType::MTR_BEGIN, trx_id));
}

RC CLogManager::commit_trx(int32_t trx_id, int32_t commit_xid)
{
  RC rc = append_log(CLogRecord::build_commit_record(trx_id, commit_xid));
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to append trx commit log. trx id=%d, rc=%s", trx_id, strrc(rc));
    return rc;
  }

  rc = sync(); // 事务提交时需要把当前事务关联的日志，都写入到磁盘中，这样做是保证不丢数据
  return rc;
}

RC CLogManager::rollback_trx(int32_t trx_id)
{
  return append_log(CLogRecord::build_mtr_record(CLogType::MTR_ROLLBACK, trx_id));
}

RC CLogManager::append_log(CLogRecord *log_record)
{
  if (nullptr == log_record) {
    return RC::INVALID_ARGUMENT;
  }
  return log_buffer_->append_log_record(log_record);
}

RC CLogManager::sync()
{
  return log_buffer_->flush_buffer(*log_file_);
}

RC CLogManager::recover(Db *db)
{
  CLogRecordIterator log_record_iterator;
  RC rc = log_record_iterator.init(*log_file_);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to init log record iterator. rc=%s", strrc(rc));
    return rc;
  }

  TrxKit *trx_manager = GCTX.trx_kit_;
  ASSERT(trx_manager != nullptr, "cannot do recover that trx_manager is null");

  /// 遍历所有的日志，然后做redo
  // 在做redo时，需要记录处理的事务。在所有的日志都重做完成时，如果有事务没有结束，那这些事务就需要回滚
  for (rc = log_record_iterator.next(); OB_SUCC(rc) && log_record_iterator.valid(); rc = log_record_iterator.next()) {
    const CLogRecord &log_record = log_record_iterator.log_record();
    LOG_TRACE("begin to redo log={%s}", log_record.to_string().c_str());
    switch (log_record.log_type()) {
      case CLogType::MTR_BEGIN: {
        Trx *trx = trx_manager->create_trx(log_record.trx_id());
        if (trx == nullptr) {
          LOG_WARN("failed to create trx. log_record={%s}", log_record.to_string().c_str());
          return RC::INTERNAL;
        }
      } break;

      case CLogType::MTR_COMMIT: 
      case CLogType::MTR_ROLLBACK: {
        Trx *trx = trx_manager->find_trx(log_record.trx_id());
        if (nullptr == trx) {
          LOG_WARN("no such trx. trx id=%d, log_record={%s}", log_record.trx_id(), log_record.to_string().c_str());
          return RC::INTERNAL;
        }
        rc = trx->redo(db, log_record);
        if (OB_FAIL(rc)) {
          LOG_WARN("failed to redo log. trx id=%d, log_record={%s}, rc=%s", 
                   log_record.trx_id(), log_record.to_string().c_str(), strrc(rc));
          return rc;
        }

      } break;

      default: {
        Trx *trx = GCTX.trx_kit_->find_trx(log_record.trx_id());
        ASSERT(trx != nullptr,
              "cannot find such trx. trx id=%d, log_record={%s}",
              log_record.trx_id(),
              log_record.to_string().c_str());
        
        rc = trx->redo(db, log_record);
        if (rc != RC::SUCCESS) {
          LOG_WARN("failed to redo log record. log_record={%s}, rc=%s", log_record.to_string().c_str(), strrc(rc));
          return rc;
        }

        LOG_TRACE("redo one data record done");
      } break;
    }
  }

  if (rc == RC::RECORD_EOF) {
    rc = RC::SUCCESS;
  } else {
    LOG_ERROR("failed to redo log iterator. rc=%s", strrc(rc));
    return rc;
  }

  LOG_TRACE("recover redo log done");

  vector<Trx *> uncommitted_trxes;
  trx_manager->all_trxes(uncommitted_trxes);
  LOG_INFO("find %d uncommitted trx", uncommitted_trxes.size());
  for (Trx *trx : uncommitted_trxes) {
    trx->rollback();
    trx_manager->destroy_trx(trx);
  }

  return RC::SUCCESS;
}
