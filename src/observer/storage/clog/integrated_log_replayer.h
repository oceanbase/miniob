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
#include "storage/index/bplus_tree_log.h"
#include "storage/trx/mvcc_trx_log.h"

class BufferPoolManager;

/**
 * @brief 整体日志回放类
 * @ingroup Clog
 * @details 负责回放所有日志，是其它各模块日志回放的分发器
 */
class IntegratedLogReplayer : public LogReplayer
{
public:
  /**
   * @brief 构造函数
   * @details 在做恢复时，我们通常需要一个 BufferPoolManager 对象，因为恢复过程中需要读取磁盘页。
   * BufferPoolManager 在对应MySQL中，可以类比table space 的管理器。但是在这里，一个表可能会有多个table space(buffer
   * pool)。 比如一个数据文件、多个索引文件。
   */
  IntegratedLogReplayer(BufferPoolManager &bpm);

  /**
   * @brief 构造函数
   * @details
   * 区别于另一个构造函数，这个构造函数可以指定不同的事务日志回放器。比如进程启动时可以指定选择使用VacuousTrx还是MvccTrx。
   */
  IntegratedLogReplayer(BufferPoolManager &bpm, unique_ptr<LogReplayer> trx_log_replayer);
  virtual ~IntegratedLogReplayer() = default;

  //! @copydoc LogReplayer::replay
  RC replay(const LogEntry &entry) override;

  //! @copydoc LogReplayer::on_done
  RC on_done() override;

private:
  BufferPoolLogReplayer   buffer_pool_log_replayer_;  ///< 缓冲池日志回放器
  RecordLogReplayer       record_log_replayer_;       ///< record manager 日志回放器
  BplusTreeLogReplayer    bplus_tree_log_replayer_;   ///< bplus tree 日志回放器
  unique_ptr<LogReplayer> trx_log_replayer_;          ///< trx 日志回放器
};