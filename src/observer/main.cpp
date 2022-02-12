/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

// __CR__

/*
 *  Created on: Mar 11, 2012
 *      Author: Longda Feng
 */

#include <netinet/in.h>
#include <unistd.h>
#include <iostream>

#include "init.h"
#include "common/os/process.h"
#include "common/os/signal.h"
#include "net/server.h"
#include "net/server_param.h"

using namespace common;

#define NET "NET"

static Server *g_server = nullptr;

void usage()
{
  std::cout << "Useage " << std::endl;
  std::cout << "-p: server port. if not specified, the item in the config file will be used" << std::endl;
  std::cout << "-f: path of config file." << std::endl;
  std::cout << "-s: use unix socket and the argument is socket address" << std::endl;
  exit(0);
}

void parse_parameter(int argc, char **argv)
{
  std::string process_name = get_process_name(argv[0]);

  ProcessParam *process_param = the_process_param();

  process_param->init_default(process_name);

  // Process args
  int opt;
  extern char *optarg;
  while ((opt = getopt(argc, argv, "dp:s:f:o:e:h")) > 0) {
    switch (opt) {
      case 's':
        process_param->set_unix_socket_path(optarg);
        break;
      case 'p':
        process_param->set_server_port(atoi(optarg));
        break;
      case 'f':
        process_param->set_conf(optarg);
        break;
      case 'o':
        process_param->set_std_out(optarg);
        break;
      case 'e':
        process_param->set_std_err(optarg);
        break;
      case 'd':
        process_param->set_demon(true);
        break;
      case 'h':
      default:
        usage();
        return;
    }
  }
}

Server *init_server()
{
  std::map<std::string, std::string> net_section = get_properties()->get(NET);

  ProcessParam *process_param = the_process_param();

  long listen_addr = INADDR_ANY;
  long max_connection_num = MAX_CONNECTION_NUM_DEFAULT;
  int port = PORT_DEFAULT;

  std::map<std::string, std::string>::iterator it = net_section.find(CLIENT_ADDRESS);
  if (it != net_section.end()) {
    std::string str = it->second;
    str_to_val(str, listen_addr);
  }

  it = net_section.find(MAX_CONNECTION_NUM);
  if (it != net_section.end()) {
    std::string str = it->second;
    str_to_val(str, max_connection_num);
  }

  if (process_param->get_server_port() > 0) {
    port = process_param->get_server_port();
    LOG_INFO("Use port config in command line: %d", port);
  } else {
    it = net_section.find(PORT);
    if (it != net_section.end()) {
      std::string str = it->second;
      str_to_val(str, port);
    }
  }

  ServerParam server_param;
  server_param.listen_addr = listen_addr;
  server_param.max_connection_num = max_connection_num;
  server_param.port = port;

  if (process_param->get_unix_socket_path().size() > 0) {
    server_param.use_unix_socket = true;
    server_param.unix_socket_path = process_param->get_unix_socket_path();
  }

  Server *server = new Server(server_param);
  return server;
}

/**
 * 如果收到terminal信号的时候，正在处理某些事情，比如打日志，并且拿着日志的锁
 * 那么直接在signal_handler里面处理的话，可能会导致死锁
 * 所以这里单独创建一个线程
 */
void *quit_thread_func(void *_signum)
{
  intptr_t signum = (intptr_t)_signum;
  LOG_INFO("Receive signal: %ld", signum);
  if (g_server) {
    g_server->shutdown();
    delete g_server;
    g_server = nullptr;
  }
  return nullptr;
}
void quit_signal_handle(int signum)
{
  pthread_t tid;
  pthread_create(&tid, nullptr, quit_thread_func, (void *)(intptr_t)signum);
}

int main(int argc, char **argv)
{
  setSignalHandler(quit_signal_handle);

  parse_parameter(argc, argv);

  int rc = STATUS_SUCCESS;
  rc = init(the_process_param());
  if (rc) {
    std::cerr << "Shutdown due to failed to init!" << std::endl;
    cleanup();
    return rc;
  }

  g_server = init_server();
  Server::init();
  g_server->serve();

  LOG_INFO("Server stopped");

  cleanup();
}
