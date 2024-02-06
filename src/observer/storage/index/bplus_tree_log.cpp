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
// Created by wangyunlai.wyl on 2024/02/05.
//

#include <sstream>

#include "storage/index/bplus_tree_log.h"
#include "storage/index/bplus_tree.h"

using namespace std;

string BplusTreeLogOperation::to_string() const
{
  stringstream ss;
  ss << std::to_string(index()) << ":";
  switch (type_) {
    case Type::INIT_HEADER_PAGE:
      ss << "INIT_HEADER_PAGE";
      break;
    default:
      ss << "UNKNOWN";
      break;
  }
  return ss.str();
}

///////////////////////////////////////////////////////////////////////////////
// class BplusTreeLogger
BplusTreeLogger::BplusTreeLogger(LogHandler &log_handler, int32_t buffer_pool_id, int32_t key_size, int32_t value_size)
  : log_handler_(log_handler),
    buffer_pool_id_(buffer_pool_id),
    key_size_(key_size),
    value_size_(value_size)
{
  (void)log_handler_;
  (void)buffer_pool_id_;
  (void)(key_size_);
  (void)(value_size_);
}

BplusTreeLogger::~BplusTreeLogger()
{}


///////////////////////////////////////////////////////////////////////////////
// class BplusTreeMiniTransaction
BplusTreeMiniTransaction::BplusTreeMiniTransaction(BplusTreeHandler &tree_handler)
  : latch_memo_(&tree_handler.buffer_pool()),
    logger_(tree_handler.log_handler(), tree_handler.buffer_pool().id(), tree_handler.file_header().key_length, sizeof(RID))
{}
BplusTreeMiniTransaction::BplusTreeMiniTransaction(DiskBufferPool &buffer_pool, LogHandler &log_handler, int32_t key_size, int32_t value_size)
  : latch_memo_(&buffer_pool),
    logger_(log_handler, buffer_pool.id(), key_size, value_size)
{}

BplusTreeMiniTransaction::~BplusTreeMiniTransaction()
{
  // TODO logger.flush and then memo.release
}