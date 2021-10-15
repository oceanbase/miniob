/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Longda on 2021/4/13.
//

#ifndef __SRC_OBSERVER_NET_CONNECTION_CONTEXT_H__
#define __SRC_OBSERVER_NET_CONNECTION_CONTEXT_H__

#include <event.h>
#include <ini_setting.h>

class Session;

typedef struct _ConnectionContext {
  Session *session;
  int fd;
  struct event read_event;
  pthread_mutex_t mutex;
  char addr[24];
  char buf[SOCKET_BUFFER_SIZE];
} ConnectionContext;

#endif //__SRC_OBSERVER_NET_CONNECTION_CONTEXT_H__
