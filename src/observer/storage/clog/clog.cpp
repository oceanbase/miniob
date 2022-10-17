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

#include "common/log/log.h"
#include "clog.h"

#define CLOG_INS_REC_NODATA_SIZE (sizeof(CLogInsertRecord) - sizeof(char *))
const char *CLOG_FILE_NAME = "clog";

int _align8(int size)
{
  return size / 8 * 8 + ((size % 8 == 0) ? 0 : 8);
}

CLogRecord::CLogRecord(CLogType flag, int32_t trx_id, const char *table_name /* = nullptr */, int data_len /* = 0 */,
    Record *rec /* = nullptr */)
{
  flag_ = flag;
  switch (flag) {
    case REDO_MTR_BEGIN:
    case REDO_MTR_COMMIT: {
      log_record_.mtr.hdr_.trx_id_ = trx_id;
      log_record_.mtr.hdr_.type_ = flag;
      log_record_.mtr.hdr_.logrec_len_ = sizeof(CLogMTRRecord);
      log_record_.mtr.hdr_.lsn_ = CLogManager::get_next_lsn(log_record_.mtr.hdr_.logrec_len_);
    } break;
    case REDO_INSERT: {
      if (!rec || !rec->data()) {
        LOG_ERROR("Record is null");
      } else {
        log_record_.ins.hdr_.trx_id_ = trx_id;
        log_record_.ins.hdr_.type_ = flag;
        strcpy(log_record_.ins.table_name_, table_name);
        log_record_.ins.rid_ = rec->rid();
        log_record_.ins.data_len_ = data_len;
        log_record_.ins.hdr_.logrec_len_ = _align8(CLOG_INS_REC_NODATA_SIZE + data_len);
        log_record_.ins.data_ = new char[log_record_.ins.hdr_.logrec_len_ - CLOG_INS_REC_NODATA_SIZE];
        memcpy(log_record_.ins.data_, rec->data(), data_len);
        log_record_.ins.hdr_.lsn_ = CLogManager::get_next_lsn(log_record_.ins.hdr_.logrec_len_);
      }
    } break;
    case REDO_DELETE: {
      if (!rec) {
        LOG_ERROR("Record is null");
      } else {
        log_record_.del.hdr_.trx_id_ = trx_id;
        log_record_.del.hdr_.type_ = flag;
        log_record_.del.hdr_.logrec_len_ = sizeof(CLogDeleteRecord);
        strcpy(log_record_.ins.table_name_, table_name);
        log_record_.del.rid_ = rec->rid();
        log_record_.del.hdr_.lsn_ = CLogManager::get_next_lsn(log_record_.del.hdr_.logrec_len_);
      }
    } break;
    default:
      LOG_ERROR("flag is error");
      break;
  }
}

CLogRecord::CLogRecord(char *data)
{
  CLogRecordHeader *hdr = (CLogRecordHeader *)data;
  flag_ = (CLogType)hdr->type_;
  switch (flag_) {
    case REDO_MTR_BEGIN:
    case REDO_MTR_COMMIT: {
      log_record_.mtr.hdr_ = *hdr;
    } break;
    case REDO_INSERT: {
      log_record_.ins.hdr_ = *hdr;
      data += sizeof(CLogRecordHeader);
      strcpy(log_record_.ins.table_name_, data);
      data += TABLE_NAME_MAX_LEN;
      log_record_.ins.rid_ = *(RID *)data;
      data += sizeof(RID);
      log_record_.ins.data_len_ = *(int *)data;
      data += sizeof(int);
      log_record_.ins.data_ = new char[log_record_.ins.hdr_.logrec_len_ - CLOG_INS_REC_NODATA_SIZE];
      memcpy(log_record_.ins.data_, data, log_record_.ins.data_len_);
    } break;
    case REDO_DELETE: {
      log_record_.del.hdr_ = *hdr;
      data += sizeof(CLogRecordHeader);
      strcpy(log_record_.del.table_name_, data);
      data += TABLE_NAME_MAX_LEN;
      log_record_.del.rid_ = *(RID *)data;
    } break;
    default:
      LOG_ERROR("flag is error");
      break;
  }
}

CLogRecord::~CLogRecord()
{
  if (REDO_INSERT == flag_) {
    delete[] log_record_.ins.data_;
  }
}

RC CLogRecord::copy_record(void *dest, int start_off, int copy_len)
{
  CLogRecords *log_rec = &log_record_;
  if (start_off + copy_len > get_logrec_len()) {
    return RC::GENERIC_ERROR;
  } else if (flag_ != REDO_INSERT) {
    memcpy(dest, (char *)log_rec + start_off, copy_len);
  } else {
    if (start_off > CLOG_INS_REC_NODATA_SIZE) {
      memcpy(dest, log_rec->ins.data_ + start_off - CLOG_INS_REC_NODATA_SIZE, copy_len);
    } else if (start_off + copy_len <= CLOG_INS_REC_NODATA_SIZE) {
      memcpy(dest, (char *)log_rec + start_off, copy_len);
    } else {
      memcpy(dest, (char *)log_rec + start_off, CLOG_INS_REC_NODATA_SIZE - start_off);
      memcpy((char *)dest + CLOG_INS_REC_NODATA_SIZE - start_off,
          log_rec->ins.data_,
          copy_len - (CLOG_INS_REC_NODATA_SIZE - start_off));
    }
  }
  return RC::SUCCESS;
}

// for unitest // 1 = "="// 0 = "!="
int CLogRecord::cmp_eq(CLogRecord *other)
{
  CLogRecords *other_logrec = other->get_record();
  if (flag_ == other->flag_) {
    switch (flag_) {
      case REDO_MTR_BEGIN:
      case REDO_MTR_COMMIT:
        return log_record_.mtr == other_logrec->mtr;
      case REDO_INSERT:
        return log_record_.ins == other_logrec->ins;
      case REDO_DELETE:
        return log_record_.del == other_logrec->del;
      default:
        LOG_ERROR("log_record is error");
        break;
    }
  }
  return 0;
}

//
CLogBuffer::CLogBuffer()
{
  current_block_no_ = 0 * CLOG_BLOCK_SIZE;  // 第一个块是文件头块
  write_block_offset_ = 0;
  write_offset_ = 0;
  memset(buffer_, 0, CLOG_BUFFER_SIZE);
}

CLogBuffer::~CLogBuffer()
{}

RC CLogBuffer::append_log_record(CLogRecord *log_rec, int &start_off)
{
  if (!log_rec) {
    return RC::GENERIC_ERROR;
  }
  if (write_offset_ == CLOG_BUFFER_SIZE) {
    return RC::LOGBUF_FULL;
  }
  RC rc = RC::SUCCESS;
  int32_t logrec_left_len = log_rec->get_logrec_len() - start_off;
  CLogBlock *log_block = (CLogBlock *)&buffer_[write_block_offset_];

  if (write_offset_ == 0 && write_block_offset_ == 0) {
    memset(log_block, 0, CLOG_BLOCK_SIZE);
    current_block_no_ += CLOG_BLOCK_SIZE;
    log_block->log_block_hdr_.log_block_no = current_block_no_;
    write_offset_ += CLOG_BLOCK_HDR_SIZE;
  }
  if (log_block->log_block_hdr_.log_data_len_ == CLOG_BLOCK_DATA_SIZE) {  // 当前block已写满
    // 新分配一个block
    write_block_offset_ += CLOG_BLOCK_SIZE;
    current_block_no_ += CLOG_BLOCK_SIZE;
    log_block = (CLogBlock *)&buffer_[write_block_offset_];
    memset(log_block, 0, CLOG_BLOCK_SIZE);
    log_block->log_block_hdr_.log_block_no = current_block_no_;
    write_offset_ += CLOG_BLOCK_HDR_SIZE;
    return append_log_record(log_rec, start_off);
  } else {
    if (logrec_left_len <= (CLOG_BLOCK_DATA_SIZE - log_block->log_block_hdr_.log_data_len_)) {  //不需要再跨block存放
      if (log_block->log_block_hdr_.log_data_len_ == 0) {                                       //当前为新block
        if (start_off == 0) {
          log_block->log_block_hdr_.first_rec_offset_ = CLOG_BLOCK_HDR_SIZE;
        } else {
          log_block->log_block_hdr_.first_rec_offset_ = CLOG_BLOCK_HDR_SIZE + logrec_left_len;
        }
      }
      log_rec->copy_record(&(buffer_[write_offset_]), start_off, logrec_left_len);
      write_offset_ += logrec_left_len;
      log_block->log_block_hdr_.log_data_len_ += logrec_left_len;
      start_off += logrec_left_len;
    } else {                                               //需要跨block
      if (log_block->log_block_hdr_.log_data_len_ == 0) {  //当前为新block
        log_block->log_block_hdr_.first_rec_offset_ = CLOG_BLOCK_SIZE;
      }
      int32_t block_left_len = CLOG_BLOCK_DATA_SIZE - log_block->log_block_hdr_.log_data_len_;
      log_rec->copy_record(&(buffer_[write_offset_]), start_off, block_left_len);
      write_offset_ += block_left_len;
      log_block->log_block_hdr_.log_data_len_ += block_left_len;
      start_off += block_left_len;
      return append_log_record(log_rec, start_off);
    }
  }
  return rc;
}

RC CLogBuffer::flush_buffer(CLogFile *log_file)
{
  if (write_offset_ == CLOG_BUFFER_SIZE) {  //如果是buffer满触发的下刷
    CLogBlock *log_block = (CLogBlock *)buffer_;
    log_file->write(log_block->log_block_hdr_.log_block_no, CLOG_BUFFER_SIZE, buffer_);
    write_block_offset_ = 0;
    write_offset_ = 0;
    memset(buffer_, 0, CLOG_BUFFER_SIZE);
  } else {
    CLogBlock *log_block = (CLogBlock *)buffer_;
    log_file->write(log_block->log_block_hdr_.log_block_no, write_block_offset_ + CLOG_BLOCK_SIZE, buffer_);
    log_block = (CLogBlock *)&buffer_[write_block_offset_];
    if (log_block->log_block_hdr_.log_data_len_ == CLOG_BLOCK_DATA_SIZE) {  // 最后一个block已写满
      write_block_offset_ = 0;
      write_offset_ = 0;
      memset(buffer_, 0, CLOG_BUFFER_SIZE);
    } else if (write_block_offset_ != 0) {
      // 将最后一个未写满的block迁移到buffer起始位置
      write_offset_ = log_block->log_block_hdr_.log_data_len_ + CLOG_BLOCK_HDR_SIZE;
      memcpy(buffer_, &buffer_[write_block_offset_], CLOG_BLOCK_SIZE);
      write_block_offset_ = 0;
      memset(&buffer_[CLOG_BLOCK_SIZE], 0, CLOG_BUFFER_SIZE - CLOG_BLOCK_SIZE);
    }
  }
  return RC::SUCCESS;
}

RC CLogBuffer::block_copy(int32_t offset, CLogBlock *log_block)
{
  memcpy(&buffer_[offset], (char *)log_block, CLOG_BLOCK_SIZE);
  return RC::SUCCESS;
}

//
CLogFile::CLogFile(const char *path)
{
  log_file_ = new PersistHandler();
  RC rc = RC::SUCCESS;
  std::string clog_file_path = std::string(path) + common::FILE_PATH_SPLIT_STR + CLOG_FILE_NAME;
  rc = log_file_->create_file(clog_file_path.c_str());
  if (rc == RC::SUCCESS) {
    log_file_->open_file();
    update_log_fhd(0);
  } else if (rc == RC::FILE_EXIST) {
    log_file_->open_file(clog_file_path.c_str());
    log_file_->read_at(0, CLOG_BLOCK_SIZE, (char *)&log_fhd_);
  }
}

CLogFile::~CLogFile()
{
  if (log_file_) {
    log_file_->close_file();
    delete log_file_;
  }
}

RC CLogFile::update_log_fhd(int32_t current_file_lsn)
{
  log_fhd_.hdr_.current_file_lsn_ = current_file_lsn;
  log_fhd_.hdr_.current_file_real_offset_ = CLOG_FILE_HDR_SIZE;
  RC rc = log_file_->write_at(0, CLOG_BLOCK_SIZE, (char *)&log_fhd_);
  return rc;
}

RC CLogFile::append(int data_len, char *data)
{
  RC rc = log_file_->append(data_len, data);
  return rc;
}

RC CLogFile::write(uint64_t offset, int data_len, char *data)
{
  RC rc = log_file_->write_at(offset, data_len, data);
  return rc;
}

RC CLogFile::recover(CLogMTRManager *mtr_mgr, CLogBuffer *log_buffer)
{
  char redo_buffer[CLOG_REDO_BUFFER_SIZE];
  CLogRecordBuf logrec_buf;
  memset(&logrec_buf, 0, sizeof(CLogRecordBuf));
  CLogRecord *log_rec = nullptr;

  uint64_t offset = CLOG_BLOCK_SIZE;  // 第一个block为文件头
  int64_t read_size = 0;
  log_file_->read_at(offset, CLOG_REDO_BUFFER_SIZE, redo_buffer, &read_size);
  while (read_size != 0) {
    int32_t buffer_offset = 0;
    while (buffer_offset < CLOG_REDO_BUFFER_SIZE) {
      CLogBlock *log_block = (CLogBlock *)&redo_buffer[buffer_offset];
      log_buffer->set_current_block_no(log_block->log_block_hdr_.log_block_no);

      int16_t rec_offset = CLOG_BLOCK_HDR_SIZE;
      while (rec_offset < CLOG_BLOCK_HDR_SIZE + log_block->log_block_hdr_.log_data_len_) {
        block_recover(log_block, rec_offset, &logrec_buf, log_rec);
        if (log_rec != nullptr) {
          CLogManager::gloabl_lsn_ = log_rec->get_lsn() + log_rec->get_logrec_len();
          mtr_mgr->log_record_manage(log_rec);
          log_rec = nullptr;
        }
      }

      if (log_block->log_block_hdr_.log_data_len_ < CLOG_BLOCK_DATA_SIZE) {  //最后一个block
        log_buffer->block_copy(0, log_block);
        log_buffer->set_write_block_offset(0);
        log_buffer->set_write_offset(log_block->log_block_hdr_.log_data_len_ + CLOG_BLOCK_HDR_SIZE);
        goto done;
      }
      buffer_offset += CLOG_BLOCK_SIZE;
    }
    offset += read_size;
    log_file_->read_at(offset, CLOG_REDO_BUFFER_SIZE, redo_buffer, &read_size);
  }

done:
  if (logrec_buf.write_offset_ != 0) {
    log_rec = new CLogRecord((char *)logrec_buf.buffer_);
    mtr_mgr->log_record_manage(log_rec);
  }
  return RC::SUCCESS;
}

RC CLogFile::block_recover(CLogBlock *block, int16_t &offset, CLogRecordBuf *logrec_buf, CLogRecord *&log_rec)
{
  if (offset == CLOG_BLOCK_HDR_SIZE &&
      block->log_block_hdr_.first_rec_offset_ != CLOG_BLOCK_HDR_SIZE) {  //跨block中的某部分（非第一部分）
    // 追加到logrec_buf
    memcpy(&logrec_buf->buffer_[logrec_buf->write_offset_],
        (char *)block + (int)offset,
        block->log_block_hdr_.first_rec_offset_ - CLOG_BLOCK_HDR_SIZE);
    logrec_buf->write_offset_ += block->log_block_hdr_.first_rec_offset_ - CLOG_BLOCK_HDR_SIZE;
    offset += block->log_block_hdr_.first_rec_offset_ - CLOG_BLOCK_HDR_SIZE;
  } else {
    if (CLOG_BLOCK_SIZE - offset < sizeof(CLogRecordHeader)) {  // 一定是跨block的第一部分
      // 此时无法确定log record的长度
      // 开始写入logrec_buf
      memcpy(&logrec_buf->buffer_[logrec_buf->write_offset_], (char *)block + (int)offset, CLOG_BLOCK_SIZE - offset);
      logrec_buf->write_offset_ += CLOG_BLOCK_SIZE - offset;
      offset = CLOG_BLOCK_SIZE;
    } else {
      if (logrec_buf->write_offset_ != 0) {
        log_rec = new CLogRecord((char *)logrec_buf->buffer_);
        memset(logrec_buf, 0, sizeof(CLogRecordBuf));
      } else {
        CLogRecordHeader *logrec_hdr = (CLogRecordHeader *)((char *)block + (int)offset);
        if (logrec_hdr->logrec_len_ <= CLOG_BLOCK_SIZE - offset) {
          log_rec = new CLogRecord((char *)block + (int)offset);
          offset += logrec_hdr->logrec_len_;
        } else {  //此时为跨block的第一部分
          // 开始写入logrec_buf
          memcpy(
              &logrec_buf->buffer_[logrec_buf->write_offset_], (char *)block + (int)offset, CLOG_BLOCK_SIZE - offset);
          logrec_buf->write_offset_ += CLOG_BLOCK_SIZE - offset;
          offset = CLOG_BLOCK_SIZE;
        }
      }
    }
  }
  return RC::SUCCESS;
}
//
void CLogMTRManager::log_record_manage(CLogRecord *log_rec)
{
  if (log_rec->get_log_type() == REDO_MTR_COMMIT) {
    trx_commited[log_rec->get_trx_id()] = true;
    delete log_rec;
  } else if (log_rec->get_log_type() == REDO_MTR_BEGIN) {
    trx_commited.insert({log_rec->get_trx_id(), false});
    delete log_rec;
  } else {
    log_redo_list.push_back(log_rec);
  }
}

////////////////////
std::atomic<int32_t> CLogManager::gloabl_lsn_(0);
CLogManager::CLogManager(const char *path)
{
  log_buffer_ = new CLogBuffer();
  log_file_ = new CLogFile(path);
  log_mtr_mgr_ = new CLogMTRManager();
}

CLogManager::~CLogManager()
{
  if (log_buffer_) {
    delete log_buffer_;
  }
}

RC CLogManager::clog_gen_record(CLogType flag, int32_t trx_id, CLogRecord *&log_rec,
    const char *table_name /* = nullptr */, int data_len /* = 0 */, Record *rec /* = nullptr*/)
{
  CLogRecord *log_record = new CLogRecord(flag, trx_id, table_name, data_len, rec);
  if (log_record) {
    log_rec = log_record;
  } else {
    LOG_ERROR("new CLogRecord failed");
    return RC::NOMEM;
  }
  return RC::SUCCESS;
}

RC CLogManager::clog_append_record(CLogRecord *log_rec)
{
  RC rc = RC::SUCCESS;
  int start_offset = 0;
  rc = log_buffer_->append_log_record(log_rec, start_offset);
  if (rc == RC::LOGBUF_FULL || log_rec->get_log_type() == REDO_MTR_COMMIT) {
    clog_sync();
    if (start_offset != log_rec->get_logrec_len()) {  // 当前日志记录还没写完
      log_buffer_->append_log_record(log_rec, start_offset);
    }
  }
  delete log_rec;  // NOTE: 单元测试需要注释该行
  return rc;
}

RC CLogManager::clog_sync()
{
  RC rc = RC::SUCCESS;
  rc = log_buffer_->flush_buffer(log_file_);
  return rc;
}

RC CLogManager::recover()
{
  log_file_->recover(log_mtr_mgr_, log_buffer_);
  return RC::SUCCESS;
}

CLogMTRManager *CLogManager::get_mtr_manager()
{
  return log_mtr_mgr_;
}

int32_t CLogManager::get_next_lsn(int32_t rec_len)
{
  int32_t res_lsn = CLogManager::gloabl_lsn_;
  CLogManager::gloabl_lsn_ += rec_len;  // 当前不考虑溢出
  return res_lsn;
}
