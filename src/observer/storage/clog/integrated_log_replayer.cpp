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
// Created by wangyunlai on 2024/02/04
//

#include "storage/clog/integrated_log_replayer.h"
#include "storage/clog/log_entry.h"

IntegratedLogReplayer::IntegratedLogReplayer(BufferPoolManager &bpm) 
  : buffer_pool_log_replayer_(bpm), record_log_replayer_(bpm) 
{}

RC IntegratedLogReplayer::replay(const LogEntry &entry)
{
  switch (entry.module().id()) {
    case LogModule::Id::BUFFER_POOL:
      return buffer_pool_log_replayer_.replay(entry);
    case LogModule::Id::RECORD_MANAGER:
      return record_log_replayer_.replay(entry);
    default:
      return RC::INVALID_ARGUMENT;
  }
}