/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
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

#include "common/ini_setting.h"
#include "common/init.h"
#include "common/lang/iostream.h"
#include "common/lang/string.h"
#include "common/lang/map.h"
#include "common/os/process.h"
#include "common/os/signal.h"
#include "common/log/log.h"
#include "net/server.h"
#include "net/server_param.h"

using namespace common;

#define NET "NET"

static Server *g_server = nullptr;

void usage()
{
  cout << "Usage " << endl;
  cout << "-p: server port. if not specified, the item in the config file will be used" << endl;
  cout << "-f: path of config file." << endl;
  cout << "-s: use unix socket and the argument is socket address" << endl;
  cout << "-P: protocol. {plain(default), mysql, cli}." << endl;
  cout << "-t: transaction model. {vacuous(default), mvcc}." << endl;
  cout << "-T: thread handling model. {one-thread-per-connection(default),java-thread-pool}." << endl;
  cout << "-n: buffer pool memory size in byte" << endl;
  cout << "-d: durbility mode. {vacuous(default), disk}" << endl;
}

void parse_parameter(int argc, char **argv)
{
  string process_name = get_process_name(argv[0]);

  ProcessParam *process_param = the_process_param();

  process_param->init_default(process_name);

  // Process args
  int          opt;
  extern char *optarg;
  while ((opt = getopt(argc, argv, "dp:P:s:t:T:f:o:e:hn:")) > 0) {
    switch (opt) {
      case 's': process_param->set_unix_socket_path(optarg); break;
      case 'p': process_param->set_server_port(atoi(optarg)); break;
      case 'P': process_param->set_protocol(optarg); break;
      case 'f': process_param->set_conf(optarg); break;
      case 'o': process_param->set_std_out(optarg); break;
      case 'e': process_param->set_std_err(optarg); break;
      case 't': process_param->set_trx_kit_name(optarg); break;
      case 'T': process_param->set_thread_handling_name(optarg); break;
      case 'n': process_param->set_buffer_pool_memory_size(atoi(optarg)); break;
      case 'd': process_param->set_durability_mode("disk"); break;
      case 'h':
        usage();
        exit(0);
        return;
      default: cout << "Unknown option: " << static_cast<char>(opt) << ", ignored" << endl; break;
    }
  }
}

Server *init_server()
{
  map<string, string> net_section = get_properties()->get(NET);

  ProcessParam *process_param = the_process_param();

  long listen_addr        = INADDR_ANY;
  long max_connection_num = MAX_CONNECTION_NUM_DEFAULT;
  int  port               = PORT_DEFAULT;

  map<string, string>::iterator it = net_section.find(CLIENT_ADDRESS);
  if (it != net_section.end()) {
    string str = it->second;
    str_to_val(str, listen_addr);
  }

  it = net_section.find(MAX_CONNECTION_NUM);
  if (it != net_section.end()) {
    string str = it->second;
    str_to_val(str, max_connection_num);
  }

  if (process_param->get_server_port() > 0) {
    port = process_param->get_server_port();
    LOG_INFO("Use port config in command line: %d", port);
  } else {
    it = net_section.find(PORT);
    if (it != net_section.end()) {
      string str = it->second;
      str_to_val(str, port);
    }
  }

  ServerParam server_param;
  server_param.listen_addr        = listen_addr;
  server_param.max_connection_num = max_connection_num;
  server_param.port               = port;
  if (0 == strcasecmp(process_param->get_protocol().c_str(), "mysql")) {
    server_param.protocol = CommunicateProtocol::MYSQL;
  } else if (0 == strcasecmp(process_param->get_protocol().c_str(), "cli")) {
    server_param.use_std_io = true;
    server_param.protocol   = CommunicateProtocol::CLI;
  } else {
    server_param.protocol = CommunicateProtocol::PLAIN;
  }

  if (process_param->get_unix_socket_path().size() > 0 && !server_param.use_std_io) {
    server_param.use_unix_socket  = true;
    server_param.unix_socket_path = process_param->get_unix_socket_path();
  }
  server_param.thread_handling = process_param->thread_handling_name();

  Server *server = nullptr;
  if (server_param.use_std_io) {
    server = new CliServer(server_param);
  } else {
    server = new NetServer(server_param);
  }

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
  }
  return nullptr;
}
void quit_signal_handle(int signum)
{
  // 防止多次调用退出
  // 其实正确的处理是，应该全局性的控制来防止出现“多次”退出的状态，包括发起信号
  // 退出与进程主动退出
  set_signal_handler(nullptr);

  pthread_t tid;
  pthread_create(&tid, nullptr, quit_thread_func, (void *)(intptr_t)signum);
}

const char *startup_tips = R"(
Welcome to the OceanBase database implementation course.

Copyright (c) 2021 OceanBase and/or its affiliates.

Learn more about OceanBase at https://github.com/oceanbase/oceanbase
Learn more about MiniOB at https://github.com/oceanbase/miniob

)";

int main(int argc, char **argv)
{
  int rc = STATUS_SUCCESS;

  cout << startup_tips;

  set_signal_handler(quit_signal_handle);

  parse_parameter(argc, argv);

  rc = init(the_process_param());
  if (rc != STATUS_SUCCESS) {
    cerr << "Shutdown due to failed to init!" << endl;
    cleanup();
    return rc;
  }

  g_server = init_server();
  g_server->serve();

  LOG_INFO("Server stopped");

  cleanup();

  delete g_server;
  return 0;
}
