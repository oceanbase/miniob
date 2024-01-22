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

#pragma once

#include <functional>
#include "common/rc.h"

class Communicator;

/**
 * @defgroup  ThreadHandler
 * @brief 线程池处理模型接口
 * @details 处理连接上所有的消息。可以使用不同的模型来处理，当前有一个连接一个线程的模式和线程池模式。
 * 线程模型仅处理与客户端通讯的连接，不处理observer监听套接字。
 */
class ThreadHandler
{
public:
  ThreadHandler()          = default;
  virtual ~ThreadHandler() = default;

  /**
   * @brief 启动线程模型
   */
  virtual RC start() = 0;

  /**
   * @brief 停止线程模型
   */
  virtual RC stop() = 0;

  /**
   * @brief 等待线程模型停止
   */
  virtual RC await_stop() = 0;

  /**
   * @brief 有新的连接到达时，调用此接口
   * @param communicator 与客户端通讯的对象
   */
  virtual RC new_connection(Communicator *communicator) = 0;

  /**
   * @brief 连接断开时，调用此接口。通常都是内部调用
   * @param communicator 与客户端通讯的对象
   */
  virtual RC close_connection(Communicator *communicator) = 0;

public:
  /**
   * @brief 创建一个线程模型
   */
  static ThreadHandler *create(const char *name);
};
