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
// Created by Longda on 2021/4/1.
//

#ifndef __OBSERVER_NET_SERVER_H__
#define __OBSERVER_NET_SERVER_H__

#include "common/defs.h"
#include "common/metrics/metrics.h"
#include "common/seda/stage.h"
#include "net/connection_context.h"
#include "net/server_param.h"

class Server {
public:
  Server(ServerParam input_server_param);
  ~Server();

public:
  static void init();
  static int send(ConnectionContext *client, const char *buf, int data_len);

public:
  int serve();
  void shutdown();

private:
  static void accept(int fd, short ev, void *arg);
  // close connection
  static void close_connection(ConnectionContext *client_context);
  static void recv(int fd, short ev, void *arg);

private:
  int set_non_block(int fd);
  int start();
  int start_tcp_server();
  int start_unix_socket_server();

private:
  bool started_;

  int server_socket_;
  struct event_base *event_base_;
  struct event *listen_ev_;

  ServerParam server_param_;

  static common::Stage *session_stage_;
  static common::SimpleTimer *read_socket_metric_;
  static common::SimpleTimer *write_socket_metric_;
};

class Communicator {
public:
  virtual ~Communicator() = default;
  virtual int init(const ServerParam &server_param) = 0;
  virtual int start() = 0;
  virtual int stop() = 0;
};

#endif //__OBSERVER_NET_SERVER_H__
