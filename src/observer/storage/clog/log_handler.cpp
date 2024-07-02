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

#include <string.h>

#include "storage/clog/log_handler.h"
#include "common/lang/string.h"
#include "storage/clog/disk_log_handler.h"
#include "storage/clog/vacuous_log_handler.h"

RC LogHandler::append(LSN &lsn, LogModule::Id module, span<const char> data)
{
  vector<char> data_vec(data.begin(), data.end());
  return append(lsn, module, std::move(data_vec));
}

RC LogHandler::append(LSN &lsn, LogModule::Id module, vector<char> &&data)
{
  return _append(lsn, LogModule(module), std::move(data));
}

RC LogHandler::create(const char *name, LogHandler *&log_handler)
{
  if (name == nullptr || common::is_blank(name)) {
    name = "vacuous";
  }

  if (strcasecmp(name, "disk") == 0) {
    log_handler = new DiskLogHandler();
  } else if (strcasecmp(name, "vacuous") == 0) {
    log_handler = new VacuousLogHandler();
  } else {
    return RC::INVALID_ARGUMENT;
  }
  return RC::SUCCESS;
}