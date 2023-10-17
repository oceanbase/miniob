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
// Created by Wangyunlai on 2023/6/29.
//

#include <stdarg.h>

#include "event/sql_debug.h"
#include "session/session.h"
#include "event/session_event.h"

using namespace std;

void SqlDebug::add_debug_info(const std::string &debug_info)
{
  debug_infos_.push_back(debug_info);
}

void SqlDebug::clear_debug_info()
{
  debug_infos_.clear();
}

const list<string> &SqlDebug::get_debug_infos() const
{
  return debug_infos_;
}

void sql_debug(const char *fmt, ...)
{
  Session *session  = Session::current_session();
  if (nullptr == session) {
    return;
  }

  SessionEvent *request = session->current_request();
  if (nullptr == request) {
    return ;
  }

  SqlDebug &sql_debug = request->sql_debug();

  const int buffer_size = 4096;
  char *str = new char[buffer_size];

  va_list ap;
  va_start(ap, fmt);
  vsnprintf(str, buffer_size, fmt, ap);
  va_end(ap);

  sql_debug.add_debug_info(str);
  LOG_DEBUG("sql debug info: [%s]", str);

  delete[] str;
}
