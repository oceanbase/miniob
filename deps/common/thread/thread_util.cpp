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
// Created by Wangyunlai on 2023/01/15.
//

#include <pthread.h>
#include <stdio.h>

namespace common {

int thread_set_name(const char *name)
{
  const int namelen = 16;
  char      buf[namelen];
  snprintf(buf, namelen, "%s", name);

#ifdef __APPLE__
  return pthread_setname_np(buf);
#elif __linux__
  return pthread_setname_np(pthread_self(), buf);
#endif
}

}  // namespace common