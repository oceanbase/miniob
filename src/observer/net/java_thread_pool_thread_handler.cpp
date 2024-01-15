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

struct EventCallbackAg
{
  JavaThreadPoolThreadHandler *host = nullptr;
  Communicator *communicator = nullptr;
  struct event *ev = nullptr;
};

class SqlTaskRunner : public Runnable
{
public:
  SqlTaskRunner(JavaThreadPoolThreadHandler &host, Communicator *communicator, struct event *ev) 
    : host_(host), communicator_(communicator), ev_(ev)
  {}

  virtual ~SqlTaskRunner() = default;

  void run() override
  {
    SqlTaskHandler handler;
    RC rc = handler(communicator_);
    if (RC::SUCCESS != rc) {
      LOG_ERROR("failed to handle sql task. rc=%d", rc);
      host_.close_connection(communicator_);
    } else if (0 != event_add(ev_, nullptr)) {
      LOG_ERROR("failed to add event");
      host_.close_connection(communicator_);
    }
  }
private:
  JavaThreadPoolThreadHandler &host_;
  Communicator *communicator_ = nullptr;
  struct event *ev_ = nullptr;
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

  evthread_use_pthreads();
  event_base_ = event_base_new();
  if (nullptr == event_base_) {
    LOG_ERROR("failed to create event base");
    return RC::INTERNAL;
  }

  int ret = executor_.init("SQL",  // name
                  2,     // core size
                  8,     // max size
                  60*1000, // keep alive time
                  unique_ptr<SimpleQueue<unique_ptr<Runnable>>>(new SimpleQueue<unique_ptr<Runnable>>));
  if (0 != ret) {
    LOG_ERROR("failed to init thread pool executor");
    return RC::INTERNAL;
  }

  auto event_worker = std::bind(&JavaThreadPoolThreadHandler::event_loop_thread, this);
  ret = executor_.execute(event_worker);
  if (0 != ret) {
    LOG_ERROR("failed to execute event worker");
    return RC::INTERNAL;
  }

  return RC::SUCCESS;
}

static void event_callback(evutil_socket_t fd, short event, void *arg)
{
  if (event & (EV_READ | EV_CLOSED)) {
    EventCallbackAg *ag = (EventCallbackAg *)arg;
    JavaThreadPoolThreadHandler *handler = ag->host;
    handler->handle_event(ag);
  } else {
    LOG_ERROR("unexpected event. fd=%d, event=%d", fd, event);
  }
}

void JavaThreadPoolThreadHandler::handle_event(EventCallbackAg *ag)
{
  auto sql_handler = [this, ag]() {
    SqlTaskHandler handler;
    RC rc = handler(ag->communicator);
    if (RC::SUCCESS != rc) {
      LOG_ERROR("failed to handle sql task. rc=%d", rc);
      this->close_connection(ag->communicator);
    } else if (0 != event_add(ag->ev, nullptr)) {
      LOG_ERROR("failed to add event");
      this->close_connection(ag->communicator);
    }
  };
  
  executor_.execute(sql_handler);
}

void JavaThreadPoolThreadHandler::event_loop_thread()
{
  LOG_INFO("event base dispatch begin");
  // event_base_dispatch 仅调用一次事件循环。
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
  struct event *ev = event_new(event_base_, fd, EV_READ | EV_ET, event_callback, ag);
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
    LOG_ERROR("failed to add event");
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
    event_del(ag->ev);
    event_free(ag->ev);
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
    event_base_loopexit(event_base_, nullptr);
    executor_.shutdown();
  }

  LOG_INFO("end to stop java threadpool thread handler");
  return RC::SUCCESS;
}

RC JavaThreadPoolThreadHandler::await_stop()
{
  LOG_INFO("begin to await event base stopped");
  if (nullptr != event_base_) {
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
