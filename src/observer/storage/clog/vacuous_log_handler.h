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
// Created by wangyunlai on 2024/02/01
//

#pragma once

#include "storage/clog/log_handler.h"

/**
 * @brief VacuousLogHandler is a log handler implenmentation that do nothing in all methods.
 * It is used for testing.
 * @ingroup CLog
 */
class VacuousLogHandler : public LogHandler
{
public:
  VacuousLogHandler()          = default;
  virtual ~VacuousLogHandler() = default;

  RC init(const char *path) override { return RC::SUCCESS; }
  RC start() override { return RC::SUCCESS; }
  RC stop() override { return RC::SUCCESS; }
  RC await_termination() override { return RC::SUCCESS; }
  RC replay(LogReplayer &replayer, LSN start_lsn) override { return RC::SUCCESS; }
  RC iterate(function<RC(LogEntry &)> consumer, LSN start_lsn) override { return RC::SUCCESS; }

  RC wait_lsn(LSN lsn) override { return RC::SUCCESS; }

  LSN current_lsn() const override { return 0; }

private:
  RC _append(LSN &lsn, LogModule module, vector<char> &&) override
  {
    lsn = 0;
    return RC::SUCCESS;
  }
};