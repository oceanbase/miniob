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
// Created by Wangyunlai on 2024/02/02.
//

#include "storage/record/record_log.h"
#include "common/log/log.h"
#include "common/lang/sstream.h"
#include "common/lang/defer.h"
#include "storage/clog/log_handler.h"
#include "storage/record/record.h"
#include "storage/buffer/disk_buffer_pool.h"
#include "storage/clog/log_entry.h"
#include "storage/clog/vacuous_log_handler.h"
#include "storage/record/record_manager.h"
#include "storage/buffer/frame.h"
#include "storage/record/record_log.h"

using namespace common;

// class RecordOperation

string RecordOperation::to_string() const
{
  string ret = std::to_string(type_id()) + ":";
  switch (type_) {
    case Type::INIT_PAGE: return ret + "INIT_PAGE";
    case Type::INSERT: return ret + "INSERT";
    case Type::DELETE: return ret + "DELETE";
    case Type::UPDATE: return ret + "UPDATE";
    default: return ret + "UNKNOWN";
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// struct RecordLogHeader

const int32_t RecordLogHeader::SIZE = sizeof(RecordLogHeader);

string RecordLogHeader::to_string() const
{
  stringstream ss;
  ss << "buffer_pool_id:" << buffer_pool_id << ", operation_type:" << RecordOperation(operation_type).to_string()
     << ", page_num:" << page_num;

  switch (RecordOperation(operation_type).type()) {
    case RecordOperation::Type::INIT_PAGE: {
      ss << ", record_size:" << record_size;
    } break;
    case RecordOperation::Type::INSERT:
    case RecordOperation::Type::DELETE:
    case RecordOperation::Type::UPDATE: {
      ss << ", slot_num:" << slot_num;
    } break;
    default: {
      ss << ", unknown operation type";
    } break;
  }

  return ss.str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// class RecordLogHandler

RC RecordLogHandler::init(
    LogHandler &log_handler, int32_t buffer_pool_id, int32_t record_size, StorageFormat storage_format)
{
  RC rc = RC::SUCCESS;

  log_handler_    = &log_handler;
  buffer_pool_id_ = buffer_pool_id;
  record_size_    = record_size;
  storage_format_ = storage_format;

  return rc;
}

// data is the column index in page
RC RecordLogHandler::init_new_page(Frame *frame, PageNum page_num, span<const char> data)
{
  const int        log_payload_size = RecordLogHeader::SIZE + data.size();
  vector<char>     log_payload(log_payload_size);
  RecordLogHeader *header = reinterpret_cast<RecordLogHeader *>(log_payload.data());
  header->buffer_pool_id  = buffer_pool_id_;
  header->operation_type  = RecordOperation(RecordOperation::Type::INIT_PAGE).type_id();
  header->page_num        = page_num;
  header->record_size     = record_size_;
  header->storage_format  = static_cast<int>(storage_format_);
  header->column_num      = data.size() / sizeof(int);
  if (data.size() > 0) {
    memcpy(log_payload.data() + RecordLogHeader::SIZE, data.data(), data.size());
  }

  LSN lsn = 0;
  RC  rc  = log_handler_->append(lsn, LogModule::Id::RECORD_MANAGER, std::move(log_payload));
  if (OB_SUCC(rc) && lsn > 0) {
    frame->set_lsn(lsn);
  }
  return rc;
}

RC RecordLogHandler::insert_record(Frame *frame, const RID &rid, const char *record)
{
  const int        log_payload_size = RecordLogHeader::SIZE + record_size_;
  vector<char>     log_payload(log_payload_size);
  RecordLogHeader *header = reinterpret_cast<RecordLogHeader *>(log_payload.data());
  header->buffer_pool_id  = buffer_pool_id_;
  header->operation_type  = RecordOperation(RecordOperation::Type::INSERT).type_id();
  header->page_num        = rid.page_num;
  header->slot_num        = rid.slot_num;
  header->storage_format  = static_cast<int>(storage_format_);
  memcpy(log_payload.data() + RecordLogHeader::SIZE, record, record_size_);

  LSN lsn = 0;
  RC  rc  = log_handler_->append(lsn, LogModule::Id::RECORD_MANAGER, std::move(log_payload));
  if (OB_SUCC(rc) && lsn > 0) {
    frame->set_lsn(lsn);
  }
  return rc;
}

RC RecordLogHandler::update_record(Frame *frame, const RID &rid, const char *record)
{
  const int        log_payload_size = RecordLogHeader::SIZE + record_size_;
  vector<char>     log_payload(log_payload_size);
  RecordLogHeader *header = reinterpret_cast<RecordLogHeader *>(log_payload.data());
  header->buffer_pool_id  = buffer_pool_id_;
  header->operation_type  = RecordOperation(RecordOperation::Type::UPDATE).type_id();
  header->page_num        = rid.page_num;
  header->slot_num        = rid.slot_num;
  header->storage_format  = static_cast<int>(storage_format_);
  memcpy(log_payload.data() + RecordLogHeader::SIZE, record, record_size_);

  LSN lsn = 0;
  RC  rc  = log_handler_->append(lsn, LogModule::Id::RECORD_MANAGER, std::move(log_payload));
  if (OB_SUCC(rc) && lsn > 0) {
    frame->set_lsn(lsn);
  }
  return rc;
}

RC RecordLogHandler::delete_record(Frame *frame, const RID &rid)
{
  RecordLogHeader header;
  header.buffer_pool_id = buffer_pool_id_;
  header.operation_type = RecordOperation(RecordOperation::Type::DELETE).type_id();
  header.page_num       = rid.page_num;
  header.slot_num       = rid.slot_num;
  header.storage_format = static_cast<int>(storage_format_);

  LSN lsn = 0;
  RC  rc  = log_handler_->append(lsn,
      LogModule::Id::RECORD_MANAGER,
      span<const char>(reinterpret_cast<const char *>(&header), RecordLogHeader::SIZE));
  if (OB_SUCC(rc) && lsn > 0) {
    frame->set_lsn(lsn);
  }
  return rc;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// class RecordLogReplayer

RecordLogReplayer::RecordLogReplayer(BufferPoolManager &bpm) : bpm_(bpm) {}

RC RecordLogReplayer::replay(const LogEntry &entry)
{
  LOG_TRACE("replaying record manager log: %s", entry.to_string().c_str());

  if (entry.module().id() != LogModule::Id::RECORD_MANAGER) {
    return RC::INVALID_ARGUMENT;
  }

  if (entry.payload_size() < RecordLogHeader::SIZE) {
    LOG_WARN("invalid log entry. payload size: %d is less than record log header size %d", 
             entry.payload_size(), RecordLogHeader::SIZE);
    return RC::INVALID_ARGUMENT;
  }

  auto log_header = reinterpret_cast<const RecordLogHeader *>(entry.data());

  DiskBufferPool *buffer_pool = nullptr;
  Frame          *frame       = nullptr;
  RC              rc          = bpm_.get_buffer_pool(log_header->buffer_pool_id, buffer_pool);
  if (OB_FAIL(rc)) {
    LOG_WARN("fail to get buffer pool. buffer pool id=%d, rc=%s", log_header->buffer_pool_id, strrc(rc));
    return rc;
  }

  rc = buffer_pool->get_this_page(log_header->page_num, &frame);
  if (OB_FAIL(rc)) {
    LOG_WARN("fail to get this page. page num=%d, rc=%s", log_header->page_num, strrc(rc));
    return rc;
  }

  DEFER(buffer_pool->unpin_page(frame));

  const LSN frame_lsn = frame->lsn();

  if (frame_lsn >= entry.lsn()) {
    LOG_TRACE("page %d has been initialized, skip replaying record log. frame lsn %d, log lsn %d", 
              log_header->page_num, frame_lsn, entry.lsn());
    return RC::SUCCESS;
  }

  switch (RecordOperation(log_header->operation_type).type()) {
    case RecordOperation::Type::INIT_PAGE: {
      rc = replay_init_page(*buffer_pool, *log_header);
    } break;
    case RecordOperation::Type::INSERT: {
      rc = replay_insert(*buffer_pool, *log_header);
    } break;
    case RecordOperation::Type::DELETE: {
      rc = replay_delete(*buffer_pool, *log_header);
    } break;
    case RecordOperation::Type::UPDATE: {
      rc = replay_update(*buffer_pool, *log_header);
    } break;
    default: {
      LOG_WARN("unknown record operation type: %d", log_header->operation_type);
      return RC::INVALID_ARGUMENT;
    }
  }

  if (OB_FAIL(rc)) {
    LOG_WARN("fail to replay record log. log header: %s, rc=%s", log_header->to_string().c_str(), strrc(rc));
    return rc;
  }

  frame->set_lsn(entry.lsn());
  return RC::SUCCESS;
}

RC RecordLogReplayer::replay_init_page(DiskBufferPool &buffer_pool, const RecordLogHeader &log_header)
{
  VacuousLogHandler             vacuous_log_handler;
  unique_ptr<RecordPageHandler> record_page_handler(
      RecordPageHandler::create(StorageFormat(log_header.storage_format)));

  RC rc = record_page_handler->init_empty_page(buffer_pool,
      vacuous_log_handler,
      log_header.page_num,
      log_header.record_size,
      log_header.column_num,
      log_header.data);
  if (OB_FAIL(rc)) {
    LOG_WARN("fail to init record page handler. page num=%d, rc=%s", log_header.page_num, strrc(rc));
    return rc;
  }

  return rc;
}

RC RecordLogReplayer::replay_insert(DiskBufferPool &buffer_pool, const RecordLogHeader &log_header)
{
  VacuousLogHandler             vacuous_log_handler;
  unique_ptr<RecordPageHandler> record_page_handler(
      RecordPageHandler::create(StorageFormat(log_header.storage_format)));

  RC rc = record_page_handler->init(buffer_pool, vacuous_log_handler, log_header.page_num, ReadWriteMode::READ_WRITE);
  if (OB_FAIL(rc)) {
    LOG_WARN("fail to init record page handler. page num=%d, rc=%s", log_header.page_num, strrc(rc));
    return rc;
  }

  const char *record = log_header.data;
  RID         rid(log_header.page_num, log_header.slot_num);
  rc = record_page_handler->insert_record(record, &rid);
  if (OB_FAIL(rc)) {
    LOG_WARN("fail to recover insert record. page num=%d, slot num=%d, rc=%s", 
             log_header.page_num, log_header.slot_num, strrc(rc));
    return rc;
  }

  return rc;
}

RC RecordLogReplayer::replay_delete(DiskBufferPool &buffer_pool, const RecordLogHeader &log_header)
{
  VacuousLogHandler             vacuous_log_handler;
  unique_ptr<RecordPageHandler> record_page_handler(
      RecordPageHandler::create(StorageFormat(log_header.storage_format)));

  RC rc = record_page_handler->init(buffer_pool, vacuous_log_handler, log_header.page_num, ReadWriteMode::READ_WRITE);
  if (OB_FAIL(rc)) {
    LOG_WARN("fail to init record page handler. page num=%d, rc=%s", log_header.page_num, strrc(rc));
    return rc;
  }

  RID rid(log_header.page_num, log_header.slot_num);
  rc = record_page_handler->delete_record(&rid);
  if (OB_FAIL(rc)) {
    LOG_WARN("fail to recover delete record. page num=%d, slot num=%d, rc=%s", 
             log_header.page_num, log_header.slot_num, strrc(rc));
    return rc;
  }

  return rc;
}

RC RecordLogReplayer::replay_update(DiskBufferPool &buffer_pool, const RecordLogHeader &header)
{
  VacuousLogHandler             vacuous_log_handler;
  unique_ptr<RecordPageHandler> record_page_handler(RecordPageHandler::create(StorageFormat(header.storage_format)));

  RC rc = record_page_handler->init(buffer_pool, vacuous_log_handler, header.page_num, ReadWriteMode::READ_WRITE);
  if (OB_FAIL(rc)) {
    LOG_WARN("fail to init record page handler. page num=%d, rc=%s", header.page_num, strrc(rc));
    return rc;
  }

  RID rid(header.page_num, header.slot_num);
  rc = record_page_handler->update_record(rid, header.data);
  if (OB_FAIL(rc)) {
    LOG_WARN("fail to recover update record. page num=%d, slot num=%d, rc=%s", 
             header.page_num, header.slot_num, strrc(rc));
    return rc;
  }

  return rc;
}