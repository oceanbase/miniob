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
// Created by wangyunlai on 2022/02/01
//

#include "storage/buffer/buffer_pool_log.h"
#include "storage/buffer/disk_buffer_pool.h"
#include "storage/clog/log_handler.h"
#include "storage/clog/log_entry.h"

string BufferPoolLogEntry::to_string() const
{
  return string("buffer_pool_id=") + std::to_string(buffer_pool_id) +
         ", page_num=" + std::to_string(page_num) +
         ", operation_type=" + BufferPoolOperation(operation_type).to_string();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BufferPoolLogHandler::BufferPoolLogHandler(DiskBufferPool &buffer_pool, LogHandler &log_handler)
    : buffer_pool_(buffer_pool), log_handler_(log_handler)
{}

RC BufferPoolLogHandler::allocate_page(PageNum page_num, LSN &lsn)
{
  return append_log(BufferPoolOperation::Type::ALLOCATE, page_num, lsn);
}

RC BufferPoolLogHandler::deallocate_page(PageNum page_num, LSN &lsn)
{
  return append_log(BufferPoolOperation::Type::DEALLOCATE, page_num, lsn);
}

RC BufferPoolLogHandler::flush_page(Page &page)
{
  return log_handler_.wait_lsn(page.lsn);
}

RC BufferPoolLogHandler::append_log(BufferPoolOperation::Type type, PageNum page_num, LSN &lsn)
{
  BufferPoolLogEntry log;
  log.buffer_pool_id = buffer_pool_.id();
  log.page_num = page_num;
  log.operation_type = BufferPoolOperation(type).type_id();

  return log_handler_.append(lsn, LogModule::Id::BUFFER_POOL, span<const char>(reinterpret_cast<const char *>(&log), sizeof(log)));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BufferPoolLogReplayer
BufferPoolLogReplayer::BufferPoolLogReplayer(BufferPoolManager &bp_manager) : bp_manager_(bp_manager)
{}

RC BufferPoolLogReplayer::replay(const LogEntry &entry)
{
  if (entry.payload_size() != sizeof(BufferPoolLogEntry)) {
    LOG_ERROR("invalid buffer pool log entry. payload size=%d, expected=%d, entry=%s",
              entry.payload_size(), sizeof(BufferPoolLogEntry), entry.to_string().c_str());
    return RC::INVALID_ARGUMENT;
  }

  auto log = reinterpret_cast<const BufferPoolLogEntry *>(entry.data());

  LOG_TRACE("replay buffer pool log. entry=%s, log=%s", entry.to_string().c_str(), log->to_string().c_str());
  
  int32_t buffer_pool_id = log->buffer_pool_id;
  DiskBufferPool *buffer_pool = nullptr;
  RC rc = bp_manager_.get_buffer_pool(buffer_pool_id, buffer_pool);
  if (OB_FAIL(rc) || buffer_pool == nullptr) {
    LOG_ERROR("failed to get buffer pool. rc=%s, buffer pool=%p, log=%s, %s", 
              strrc(rc), buffer_pool, entry.to_string().c_str(), log->to_string().c_str());
    return rc;
  }

  BufferPoolOperation operation(log->operation_type);
  switch (operation.type())
  {
    case BufferPoolOperation::Type::ALLOCATE:
      return buffer_pool->redo_allocate_page(entry.lsn(), log->page_num);
    case BufferPoolOperation::Type::DEALLOCATE:
      return buffer_pool->redo_deallocate_page(entry.lsn(), log->page_num);
    default:
      LOG_ERROR("unknown buffer pool operation. operation=%s", operation.to_string().c_str());
      return RC::INTERNAL;
  }
  return RC::SUCCESS;
}