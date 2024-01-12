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

#include <map>

#include "net/thread_handler.h"
#include "common/thread/thread_pool_executor.h"

struct EventCallbackAg;

class JavaThreadPoolThreadHandler : public ThreadHandler
{
public:
  JavaThreadPoolThreadHandler() = default;
  virtual ~JavaThreadPoolThreadHandler();

  virtual RC start() override;
  virtual RC stop() override;
  virtual RC await_stop() override;

  virtual RC new_connection(Communicator *communicator) override;
  virtual RC close_connection(Communicator *communicator) override;

public:
  void handle_event(EventCallbackAg *ag);
  void event_loop_thread();
private:
  struct event_base *        event_base_ = nullptr;
  common::ThreadPoolExecutor executor_;
  std::map<Communicator *, EventCallbackAg *> event_map_;
  std::mutex lock_;
};