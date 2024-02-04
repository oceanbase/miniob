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

#pragma once

#include "storage/clog/log_replayer.h"
#include "storage/buffer/buffer_pool_log.h"
#include "storage/record/record_log.h"

class BufferPoolManager;

class IntegratedLogReplayer : public LogReplayer
{
public:
  IntegratedLogReplayer(BufferPoolManager &bpm);
  virtual ~IntegratedLogReplayer() = default;

  RC replay(const LogEntry &entry) override;

private:
  BufferPoolLogReplayer buffer_pool_log_replayer_;
  RecordLogReplayer record_log_replayer_;
};