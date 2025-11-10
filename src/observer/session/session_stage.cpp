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

#include "session_stage.h"

#include <string.h>

#include "common/conf/ini.h"
#include "common/lang/mutex.h"
#include "common/lang/string.h"
#include "common/log/log.h"
#include "event/session_event.h"
#include "event/sql_event.h"
#include "net/communicator.h"
#include "net/server.h"
#include "session/session.h"

using namespace common;

// Destructor
SessionStage::~SessionStage() {}

void SessionStage::handle_request(SessionEvent *event)
{
  const string &sql = event->query();
  if (common::is_blank(sql.c_str())) {
    return;
  }

  Session::set_current_session(event->session());
  event->session()->set_current_request(event);
  SQLStageEvent sql_event(event, sql);
}
