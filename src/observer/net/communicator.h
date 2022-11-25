/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
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
#include "rc.h"

struct ConnectionContext;
class SessionEvent;
class Session;

class Communicator {
public: 
  virtual ~Communicator();

  virtual RC init(int fd, Session *session, const std::string &addr);
  virtual RC read_event(SessionEvent *&event) = 0;
  virtual RC write_result(SessionEvent *event, bool &need_disconnect) = 0;

  Session *session() const { return session_; }
  struct event &read_event() { return read_event_; }
  const char *addr() const { return addr_.c_str(); }

protected:
  Session *session_ = nullptr;
  int fd_ = -1;
  struct event read_event_;
  std::string addr_;
};

class PlainCommunicator : public Communicator {
public: 
  RC read_event(SessionEvent *&event) override;
  RC write_result(SessionEvent *event, bool &need_disconnect) override;

private:
  RC write_state(SessionEvent *event, bool &need_disconnect);

};

enum class CommunicateProtocol
{
  PLAIN,
  MYSQL,
};

class CommunicatorFactory
{
public: 
  Communicator *create(CommunicateProtocol protocol);
};
