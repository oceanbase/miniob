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
#include <iostream>

#include "common/init.h"
#include "common/ini_setting.h"
#include "common/os/process.h"
#include "common/os/signal.h"
#include "common/lang/string.h"
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
  std::cout << "-P: protocol. {plain(default), mysql, cli}." << std::endl;
  std::cout << "-t: transaction model. {vacuous(default), mvcc}." << std::endl;
  std::cout << "-n: buffer pool memory size in byte" << std::endl;
}

void parse_parameter(int argc, char **argv)
{
  std::string process_name = get_process_name(argv[0]);

  ProcessParam *process_param = the_process_param();

  process_param->init_default(process_name);

  // Process args
  int opt;
  extern char *optarg;
  while ((opt = getopt(argc, argv, "dp:P:s:t:f:o:e:hn:")) > 0) {
    switch (opt) {
      case 's':
        process_param->set_unix_socket_path(optarg);
        break;
      case 'p':
        process_param->set_server_port(atoi(optarg));
        break;
      case 'P':
        process_param->set_protocol(optarg);
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
      case 't':
        process_param->set_trx_kit_name(optarg);
        break;
      case 'n':
        process_param->set_buffer_pool_memory_size(atoi(optarg));
        break;
      case 'h':
        usage();
        exit(0);
        return;
      default:
        std::cout << "Unknown option: " << static_cast<char>(opt) << ", ignored" << std::endl;
        break;
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
  if (0 == strcasecmp(process_param->get_protocol().c_str(), "mysql")) {
    server_param.protocol = CommunicateProtocol::MYSQL;
  } else if (0 == strcasecmp(process_param->get_protocol().c_str(), "cli")) {
    server_param.use_std_io = true;
    server_param.protocol = CommunicateProtocol::CLI;
  } else {
    server_param.protocol = CommunicateProtocol::PLAIN;
  }

  if (process_param->get_unix_socket_path().size() > 0 && !server_param.use_std_io) {
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
  int rc = STATUS_SUCCESS;

  setSignalHandler(quit_signal_handle);

  parse_parameter(argc, argv);

  rc = init(the_process_param());
  if (rc != STATUS_SUCCESS) {
    std::cerr << "Shutdown due to failed to init!" << std::endl;
    cleanup();
    return rc;
  }

  g_server = init_server();
  Server::init();
  g_server->serve();

  LOG_INFO("Server stopped");

  cleanup();

  delete g_server;
}

/**
 * @mainpage MiniOB
 * 
 * MiniOB 是 OceanBase 与华中科技大学联合开发的、面向"零"基础同学的数据库入门学习项目。
 *
 * MiniOB 设计的目标是面向在校学生、数据库从业者、爱好者，或者对基础技术有兴趣的爱好者, 整体代码量少，易于上手并学习, 是一个系统性的数据库学习项目。miniob 设置了一系列由浅入深的题目，以帮助同学们"零"基础入门, 让同学们快速了解数据库并深入学习数据库内核，期望通过相关训练之后，能够熟练掌握数据库内核模块的功能与协同关系, 并能够在使用数据库时，设计出高效的 SQL 。miniob 为了更好的学习数据库实现原理, 对诸多模块都做了简化，比如不考虑并发操作, 安全特性, 复杂的事物管理等功能。
 */
