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
  BufferPoolLogHeader header;
  header.buffer_pool_id = buffer_pool_.id();
  header.page_num = page_num;
  header.operation_type = BufferPoolOperation(type).type_id();

  unique_ptr<char[]> data(new char[sizeof(header)]);
  memcpy(data.get(), &header, sizeof(header));

  return log_handler_.append(lsn, LogModule::Id::BUFFER_POOL, std::move(data), sizeof(header));
}