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
// Created by Wangyunlai on 2024/01/10.
//

#include <string.h>

#include "net/thread_handler.h"
#include "net/one_thread_per_connection_thread_handler.h"
#include "net/java_thread_pool_thread_handler.h"
#include "common/log/log.h"
#include "common/lang/string.h"

ThreadHandler * ThreadHandler::create(const char *name)
{
  const char *default_name = "one-thread-per-connection";
  if (nullptr == name || common::is_blank(name)) {
    name = default_name;
  }

  if (0 == strcasecmp(name, default_name)) {
    return new OneThreadPerConnectionThreadHandler();
  } else if (0 == strcasecmp(name, "java-thread-pool")) {
    return new JavaThreadPoolThreadHandler();
  } else {
    LOG_ERROR("unknown thread handler: %s", name);
    return nullptr;
  }
}