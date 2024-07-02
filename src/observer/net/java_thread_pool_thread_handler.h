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
// Created by Wangyunlai on 2024/01/11.
//

#pragma once

#include "net/thread_handler.h"
#include "net/sql_task_handler.h"
#include "common/thread/thread_pool_executor.h"
#include "common/lang/mutex.h"

struct EventCallbackAg;

/**
 * @brief 简单线程池模型。使用了模拟Java线程池接口的线程池，所以脚JavaThreadPool
 * @ingroup ThreadHandler
 * @details 使用线程池处理连接上的消息。使用libevent监听连接事件。
 * libevent 是一个常用并且高效的异步事件消息库，可以阅读手册了解更多。
 * [libevent 手册](https://libevent.org/doc/index.html)
 */
class JavaThreadPoolThreadHandler : public ThreadHandler
{
public:
  JavaThreadPoolThreadHandler() = default;
  virtual ~JavaThreadPoolThreadHandler();

  //! @copydoc ThreadHandler::start
  virtual RC start() override;
  //! @copydoc ThreadHandler::stop
  virtual RC stop() override;
  //! @copydoc ThreadHandler::await_stop
  virtual RC await_stop() override;

  //! @copydoc ThreadHandler::new_connection
  virtual RC new_connection(Communicator *communicator) override;
  //! @copydoc ThreadHandler::close_connection
  virtual RC close_connection(Communicator *communicator) override;

public:
  /**
   * @brief 使用libevent处理消息时，需要有一个回调函数，这里就相当于libevent的回调函数
   *
   * @param ag 处理消息回调时的参数，比如libevent的event、连接等
   */
  void handle_event(EventCallbackAg *ag);

  /**
   * @brief libevent监听连接消息事件的回调函数
   * @details 这个函数会长时间运行在线程中，并占用线程池中的一个线程
   */
  void event_loop_thread();

private:
  mutex                                  lock_;
  struct event_base                     *event_base_ = nullptr;  /// libevent 的event_base
  common::ThreadPoolExecutor             executor_;              /// 线程池
  map<Communicator *, EventCallbackAg *> event_map_;             /// 每个连接与它关联的数据

  SqlTaskHandler sql_task_handler_;  /// SQL请求处理器
};