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

#include <sstream>

#include "storage/record/record_log.h"
#include "common/log/log.h"
#include "storage/clog/log_handler.h"
#include "storage/record/record.h"

using namespace std;
using namespace common;

// class RecordOperation

string RecordOperation::to_string() const
{
  std::string ret = std::to_string(type_id()) + ":";
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

RC RecordLogHandler::init(LogHandler &log_handler, int32_t buffer_pool_id, int32_t record_size)
{
  RC rc = RC::SUCCESS;

  log_handler_ = &log_handler;
  buffer_pool_id_ = buffer_pool_id;
  record_size_ = record_size;

  return rc;
}

RC RecordLogHandler::init_new_page(PageNum page_num, LSN &lsn)
{
  RecordLogHeader header;
  header.buffer_pool_id = buffer_pool_id_;
  header.operation_type = RecordOperation(RecordOperation::Type::INIT_PAGE).type_id();
  header.page_num       = page_num;
  header.record_size    = record_size_;

  return log_handler_->append(
      lsn, LogModule::Id::RECORD_MANAGER, reinterpret_cast<const char *>(&header), RecordLogHeader::SIZE);
}

RC RecordLogHandler::insert_record(const RID &rid, const char *record, LSN &lsn)
{
  const int          log_payload_size = RecordLogHeader::SIZE + record_size_;
  unique_ptr<char[]> log_payload(new char[log_payload_size]);
  RecordLogHeader   *header = reinterpret_cast<RecordLogHeader *>(log_payload.get());
  header->buffer_pool_id    = buffer_pool_id_;
  header->operation_type    = RecordOperation(RecordOperation::Type::INSERT).type_id();
  header->page_num          = rid.page_num;
  header->slot_num       = rid.slot_num;
  memcpy(log_payload.get() + RecordLogHeader::SIZE, record, record_size_);

  return log_handler_->append(lsn, LogModule::Id::RECORD_MANAGER, std::move(log_payload), log_payload_size);
}

RC RecordLogHandler::update_record(const RID &rid, const char *record, LSN &lsn)
{
  const int          log_payload_size = RecordLogHeader::SIZE + record_size_;
  unique_ptr<char[]> log_payload(new char[log_payload_size]);
  RecordLogHeader   *header = reinterpret_cast<RecordLogHeader *>(log_payload.get());
  header->buffer_pool_id    = buffer_pool_id_;
  header->operation_type    = RecordOperation(RecordOperation::Type::UPDATE).type_id();
  header->page_num          = rid.page_num;
  header->slot_num       = rid.slot_num;
  memcpy(log_payload.get() + RecordLogHeader::SIZE, record, record_size_);

  return log_handler_->append(lsn, LogModule::Id::RECORD_MANAGER, std::move(log_payload), log_payload_size);
}

RC RecordLogHandler::delete_record(const RID &rid, LSN &lsn)
{
  RecordLogHeader header;
  header.buffer_pool_id    = buffer_pool_id_;
  header.operation_type    = RecordOperation(RecordOperation::Type::DELETE).type_id();
  header.page_num          = rid.page_num;
  header.slot_num          = rid.slot_num;

  return log_handler_->append(lsn, LogModule::Id::RECORD_MANAGER, reinterpret_cast<const char *>(&header), RecordLogHeader::SIZE);
}