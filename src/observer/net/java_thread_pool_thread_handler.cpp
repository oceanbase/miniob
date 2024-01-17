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
// Created by Wangyunlai on 2024/01/12.
//

#include "event2/event.h"
#include "event2/thread.h"
#include "net/java_thread_pool_thread_handler.h"
#include "net/sql_task_handler.h"
#include "net/communicator.h"
#include "common/log/log.h"
#include "common/thread/runnable.h"
#include "common/queue/simple_queue.h"

using namespace std;
using namespace common;

/**
 * @brief libevent 消息回调函数的参数
 * 
 */
struct EventCallbackAg
{
  JavaThreadPoolThreadHandler *host = nullptr;
  Communicator *communicator = nullptr;
  struct event *ev = nullptr;
};

JavaThreadPoolThreadHandler::~JavaThreadPoolThreadHandler()
{
  this->stop();
  this->await_stop();
}

RC JavaThreadPoolThreadHandler::start()
{
  if (nullptr != event_base_) {
    LOG_ERROR("event base has been initialized");
    return RC::INTERNAL;
  }

  // 在多线程场景下使用libevent，先执行这个函数
  evthread_use_pthreads();
  // 创建一个event_base对象，这个对象是libevent的主要对象，所有的事件都会注册到这个对象中
  event_base_ = event_base_new();
  if (nullptr == event_base_) {
    LOG_ERROR("failed to create event base");
    return RC::INTERNAL;
  }

  // 创建线程池
  // 这里写死了线程池的大小，实际上可以从配置文件中读取
  int ret = executor_.init("SQL",  // name
                            2,     // core size
                            8,     // max size
                            60*1000 // keep alive time
                            );
  if (0 != ret) {
    LOG_ERROR("failed to init thread pool executor");
    return RC::INTERNAL;
  }

  // libevent 的监测消息循环主体，要放在一个线程中执行
  // event_loop_thread 是运行libevent 消息监测循环的函数，会长期运行，并且会放到线程池中占据一个线程
  auto event_worker = std::bind(&JavaThreadPoolThreadHandler::event_loop_thread, this);
  ret = executor_.execute(event_worker);
  if (0 != ret) {
    LOG_ERROR("failed to execute event worker");
    return RC::INTERNAL;
  }

  return RC::SUCCESS;
}

/**
 * @brief libevent 的消息事件回调函数
 * @details 当libevent检测到某个连接上有消息到达时，比如客户端发消息过来、客户端断开连接等，就会
 * 调用这个回调函数。
 * 按照libevent的描述，我们不应该在回调函数中执行比较耗时的操作，因为回调函数是运行在libevent的
 * 事件检测主循环中。
 * @param fd 有消息的连接的文件描述符
 * @param event 事件类型，比如EV_READ表示有消息到达，EV_CLOSED表示连接断开
 * @param arg 我们注册给libevent的回调函数的参数
 */
static void event_callback(evutil_socket_t fd, short event, void *arg)
{
  if (event & (EV_READ | EV_CLOSED)) {
    LOG_TRACE("got event. fd=%d, event=%d", fd, event);
    EventCallbackAg *ag = (EventCallbackAg *)arg;
    JavaThreadPoolThreadHandler *handler = ag->host;
    handler->handle_event(ag);
  } else {
    LOG_ERROR("unexpected event. fd=%d, event=%d", fd, event);
  }
}

void JavaThreadPoolThreadHandler::handle_event(EventCallbackAg *ag)
{
  /*
  当前函数是一个libevent的回调函数。按照libevent的要求，我们不能在这个函数中执行比较耗时的操作，
  因为它是运行在libevent主消息循环处理函数中的。

  我们这里在收到消息时就把它放到线程池中处理。
  */

  // sql_handler 是一个回调函数
  auto sql_handler = [this, ag]() {
    RC rc = sql_task_handler_.handle_event(ag->communicator); // 这里会有接收消息、处理请求然后返回结果一条龙服务
    if (RC::SUCCESS != rc) {
      LOG_WARN("failed to handle sql task. rc=%s", strrc(rc));
      this->close_connection(ag->communicator);
    } else if (0 != event_add(ag->ev, nullptr)) {
      // 由于我们在创建事件对象时没有增加 EV_PERSIST flag，所以我们每次都要处理完成后再把事件加回到event_base中。
      // 当然我们也不能使用 EV_PERSIST flag，否则我们在处理请求过程中，可能还会收到客户端的消息，这样就会导致并发问题。
      LOG_ERROR("failed to add event. fd=%d, communicator=%p", event_get_fd(ag->ev), this);
      this->close_connection(ag->communicator);
    } else {
      // 添加event后就不应该再访问communicator了，因为可能会有另一个线程处理当前communicator
      // 或者就需要加锁处理并发问题
      // LOG_TRACE("add event. fd=%d, communicator=%p", event_get_fd(ag->ev), this);
    }
  };
  
  executor_.execute(sql_handler);
}

void JavaThreadPoolThreadHandler::event_loop_thread()
{
  LOG_INFO("event base dispatch begin");
  // event_base_dispatch 也可以完成这个事情。
  // event_base_loop 会等待所有事件都结束
  // 如果不增加 EVLOOP_NO_EXIT_ON_EMPTY 标识，当前事件都处理完成后，就会退出循环
  // 加上这个标识就是即使没有事件，也等在这里
  event_base_loop(event_base_, EVLOOP_NO_EXIT_ON_EMPTY);
  LOG_INFO("event base dispatch end");
}

RC JavaThreadPoolThreadHandler::new_connection(Communicator *communicator)
{
  int fd = communicator->fd();
  LOG_INFO("new connection. fd=%d", fd);
  EventCallbackAg *ag = new EventCallbackAg;
  ag->host = this;
  ag->communicator = communicator;
  ag->ev = nullptr;
  /// 创建一个libevent事件对象。其中EV_READ表示可读事件，就是客户端发消息时会触发事件。
  /// EV_ET 表示边缘触发，有消息时只会触发一次，不会重复触发。这个标识在Linux平台上是支持的，但是有些平台不支持。
  /// 使用EV_ET边缘触发时需要注意一个问题，就是每次一定要把客户端发来的消息都读取完，直到read返回EAGAIN为止。
  /// 我们这里不使用边缘触发。
  /// 注意这里没有加 EV_PERSIST，表示事件触发后会自动从event_base中删除，需要自己再手动加上这个标识。这是有必
  /// 要的，因为客户端发出一个请求后，我们再返回客户端消息之前，不再希望接收新的消息。
  struct event *ev = event_new(event_base_, fd, EV_READ, event_callback, ag);
  if (nullptr == ev) {
    LOG_ERROR("failed to create event");
    return RC::INTERNAL;
  }
  ag->ev = ev;

  lock_.lock();
  event_map_[communicator] = ag;
  lock_.unlock();

  int ret = event_add(ev, nullptr);
  if (0 != ret) {
    LOG_ERROR("failed to add event. fd=%d, communicator=%p, ret=%d", fd, communicator, ret);
    event_free(ev);
    return RC::INTERNAL;
  }
  LOG_TRACE("add event success. fd=%d, communicator=%p", fd, communicator);

  return RC::SUCCESS;
}

RC JavaThreadPoolThreadHandler::close_connection(Communicator *communicator)
{
  EventCallbackAg *ag = nullptr;

  {
    lock_guard guard(lock_);
    auto iter = event_map_.find(communicator);
    if (iter == event_map_.end()) {
      LOG_ERROR("cannot find event for communicator %p", communicator);
      return RC::INTERNAL;
    }

    ag = iter->second;
    event_map_.erase(iter);
  }

  if (ag->ev) {
    event_del(ag->ev);  // 把当前事件从event_base中删除
    event_free(ag->ev); // 释放event对象
    ag->ev = nullptr;
  }
  delete ag;
  delete communicator;

  LOG_INFO("close connection. communicator = %p", communicator);
  return RC::SUCCESS;
}

RC JavaThreadPoolThreadHandler::stop()
{
  LOG_INFO("begin to stop java threadpool thread handler");

  if (nullptr != event_base_) {
    // 退出libevent的消息循环
    // 这里会一直等待libevent停止,nullptr是超时时间
    event_base_loopexit(event_base_, nullptr);

    // 停止线程池
    // libevent停止后，就不会再有新的消息到达，也不会再往线程池中添加新的任务了。
    executor_.shutdown();
  }

  LOG_INFO("end to stop java threadpool thread handler");
  return RC::SUCCESS;
}

RC JavaThreadPoolThreadHandler::await_stop()
{
  LOG_INFO("begin to await event base stopped");
  if (nullptr != event_base_) {
    // 等待线程池中所有的任务处理完成
    executor_.await_termination();
    event_base_free(event_base_);
    event_base_ = nullptr;
  }

  for (auto kv : event_map_) {
    event_free(kv.second->ev);
    delete kv.second;
    delete kv.first;
  }
  event_map_.clear();

  LOG_INFO("end to await event base stopped");
  return RC::SUCCESS;
}
