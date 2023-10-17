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
// Created by Longda on 2021/4/13.
//

#include "session_event.h"
#include "net/communicator.h"

SessionEvent::SessionEvent(Communicator *comm) 
    : communicator_(comm),
    sql_result_(communicator_->session())
{}

SessionEvent::~SessionEvent()
{
}

Communicator *SessionEvent::get_communicator() const
{
  return communicator_;
}

Session *SessionEvent::session() const
{
  return communicator_->session();
}
