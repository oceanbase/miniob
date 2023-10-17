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
// Created by Longda on 2010.
//

#include <thread>
#include <execinfo.h>

#include "common/defs.h"
#include "common/os/os.h"
#include "common/log/log.h"

namespace common {
// Don't care windows
uint32_t getCpuNum()
{
  return std::thread::hardware_concurrency();
}

#define MAX_STACK_SIZE 32

void print_stacktrace()
{
  int size = MAX_STACK_SIZE;
  void *array[MAX_STACK_SIZE];
  int stack_num = backtrace(array, size);
  char **stacktrace = backtrace_symbols(array, stack_num);
  for (int i = 0; i < stack_num; ++i) {
    LOG_INFO("%d ----- %s\n", i, stacktrace[i]);
  }
  free(stacktrace);
}

}  // namespace common