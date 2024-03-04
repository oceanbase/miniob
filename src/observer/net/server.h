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
// Created by Longda on 2021/4/1.
//

#pragma once

#include "net/server_param.h"

class Communicator;
class ThreadHandler;

/**
 * @brief 负责接收客户端消息并创建任务
 * @ingroup Communicator
 * @details 当前支持网络连接，有TCP和Unix Socket两种方式。通过命令行参数来指定使用哪种方式。
 * 启动后监听端口或unix socket，使用libevent来监听事件，当有新的连接到达时，创建一个Communicator对象进行处理。
 */
class Server
{
public:
  Server(const ServerParam &input_server_param) : server_param_(input_server_param) {}
  virtual ~Server() {}

  virtual int  serve()    = 0;
  virtual void shutdown() = 0;

protected:
  ServerParam server_param_;  ///< 服务启动参数
};

class NetServer : public Server
{
public:
  NetServer(const ServerParam &input_server_param);
  virtual ~NetServer();

public:
  int  serve() override;
  void shutdown() override;

private:
  /**
   * @brief 接收到新的连接时，调用此函数创建Communicator对象
   * @details 此函数作为libevent中监听套接字对应的回调函数
   * @param fd libevent回调函数传入的参数，即监听套接字
   * @param ev 本次触发的事件，通常是EV_READ
   */
  void accept(int fd);

private:
  /**
   * @brief 将socket描述符设置为非阻塞模式
   *
   * @param fd 指定的描述符
   */
  int set_non_block(int fd);

  int start();

  /**
   * @brief 启动TCP服务
   */
  int start_tcp_server();

  /**
   * @brief 启动Unix Socket服务
   */
  int start_unix_socket_server();

private:
  volatile bool started_ = false;

  int server_socket_ = -1;  ///< 监听套接字，是一个描述符

  CommunicatorFactory communicator_factory_;  ///< 通过这个对象创建新的Communicator对象
  ThreadHandler      *thread_handler_ = nullptr;
};

class CliServer : public Server
{
public:
  CliServer(const ServerParam &input_server_param);
  virtual ~CliServer();

  int  serve() override;
  void shutdown() override;

private:
  volatile bool started_ = false;
};