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
// Created by Wangyunlai on 2022/11/17.
//

#pragma once

#include <string>
#include <event.h>
#include "common/rc.h"

struct ConnectionContext;
class SessionEvent;
class Session;
class BufferedWriter;

/**
 * @defgroup Communicator
 * @brief 负责处理与客户端的通讯
 * @details 当前有两种通讯协议，一种是普通的文本协议，以'\0'作为结尾，一种是mysql协议。
 */

/**
 * @brief 负责与客户端通讯
 * @ingroup Communicator
 *
 * @details 在listener接收到一个新的连接(参考 server.cpp::accept), 就创建一个Communicator对象。
 * 并调用init进行初始化。
 * 在server中监听到某个连接有新的消息，就通过Communicator::read_event接收消息。

 */
class Communicator 
{
public:
  virtual ~Communicator();

  /**
   * @brief 接收到一个新的连接时，进行初始化
   */
  virtual RC init(int fd, Session *session, const std::string &addr);

  /**
   * @brief 监听到有新的数据到达，调用此函数进行接收消息
   * 如果需要创建新的任务来处理，那么就创建一个SessionEvent 对象并通过event参数返回。
   */
  virtual RC read_event(SessionEvent *&event) = 0;

  /**
   * @brief 在任务处理完成后，通过此接口将结果返回给客户端
   * @param event 任务数据，包括处理的结果
   * @param need_disconnect 是否需要断开连接
   * @return 处理结果。即使返回不是SUCCESS，也不能直接断开连接，需要通过need_disconnect来判断
   *         是否需要断开连接
   */
  virtual RC write_result(SessionEvent *event, bool &need_disconnect) = 0;

  /**
   * @brief 关联的会话信息
   */
  Session *session() const
  {
    return session_;
  }

  /**
   * @brief libevent使用的数据，参考server.cpp
   */
  struct event &read_event()
  {
    return read_event_;
  }

  /**
   * @brief 对端地址
   * 如果是unix socket，可能没有意义
   */
  const char *addr() const
  {
    return addr_.c_str();
  }

protected:
  Session *session_ = nullptr;
  struct event read_event_;
  std::string addr_;
  BufferedWriter *writer_ = nullptr;
  int fd_ = -1;
};

/**
 * @brief 当前支持的通讯协议
 * @ingroup Communicator
 */
enum class CommunicateProtocol 
{
  PLAIN,  ///< 以'\0'结尾的协议
  CLI,    ///< 与客户端进行交互的协议
  MYSQL,  ///< mysql通讯协议。具体实现参考 MysqlCommunicator
};

/**
 * @brief 通讯协议工厂
 * @ingroup Communicator
 */
class CommunicatorFactory 
{
public:
  Communicator *create(CommunicateProtocol protocol);
};
