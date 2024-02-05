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

/**
 * @brief 整体日志回放类
 * @ingroup Clog
 * @details 负责回放所有日志，是其它各模块日志回放的分发器
 */
class IntegratedLogReplayer : public LogReplayer
{
public:
  IntegratedLogReplayer(BufferPoolManager &bpm);
  virtual ~IntegratedLogReplayer() = default;

  RC replay(const LogEntry &entry) override;

private:
  BufferPoolLogReplayer buffer_pool_log_replayer_; ///< 缓冲池日志回放器
  RecordLogReplayer     record_log_replayer_;      ///< record manager 日志回放器
};