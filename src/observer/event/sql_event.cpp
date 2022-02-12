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
// Created by Longda on 2021/4/14.
//

#include "event/sql_event.h"
#include "event/session_event.h"

SQLStageEvent::SQLStageEvent(SessionEvent *event, std::string &sql) : session_event_(event), sql_(sql)
{}

SQLStageEvent::~SQLStageEvent() noexcept
{
  if (session_event_ != nullptr) {
    session_event_ = nullptr;
    // SessionEvent *session_event = session_event_;
    // session_event_ = nullptr;
    // session_event->doneImmediate();
  }
}