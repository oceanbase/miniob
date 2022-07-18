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
// Created by Longda on 2021/4/13.
//

#include <string.h>
#include <string>

#include "resolve_stage.h"

#include "common/conf/ini.h"
#include "common/io/io.h"
#include "common/lang/string.h"
#include "common/log/log.h"
#include "common/seda/timer_stage.h"
#include "event/sql_event.h"
#include "event/session_event.h"
#include "session/session.h"
#include "sql/stmt/stmt.h"

using namespace common;

//! Constructor
ResolveStage::ResolveStage(const char *tag) : Stage(tag)
{}

//! Destructor
ResolveStage::~ResolveStage()
{}

//! Parse properties, instantiate a stage object
Stage *ResolveStage::make_stage(const std::string &tag)
{
  ResolveStage *stage = new (std::nothrow) ResolveStage(tag.c_str());
  if (stage == nullptr) {
    LOG_ERROR("new ResolveStage failed");
    return nullptr;
  }
  stage->set_properties();
  return stage;
}

//! Set properties for this object set in stage specific properties
bool ResolveStage::set_properties()
{
  //  std::string stageNameStr(stage_name_);
  //  std::map<std::string, std::string> section = g_properties()->get(
  //    stageNameStr);
  //
  //  std::map<std::string, std::string>::iterator it;
  //
  //  std::string key;

  return true;
}

//! Initialize stage params and validate outputs
bool ResolveStage::initialize()
{
  LOG_TRACE("Enter");

  std::list<Stage *>::iterator stgp = next_stage_list_.begin();
  query_cache_stage_ = *(stgp++);

  LOG_TRACE("Exit");
  return true;
}

//! Cleanup after disconnection
void ResolveStage::cleanup()
{
  LOG_TRACE("Enter");

  LOG_TRACE("Exit");
}

void ResolveStage::handle_event(StageEvent *event)
{
  LOG_TRACE("Enter\n");

  SQLStageEvent *sql_event = static_cast<SQLStageEvent *>(event);
  if (nullptr == sql_event) {
    LOG_WARN("failed to get sql stage event");
    return;
  }

  SessionEvent *session_event = sql_event->session_event();

  Db *db = session_event->session()->get_current_db();
  if (nullptr == db) {
    LOG_ERROR("cannot current db");
    return ;
  }

  Query *query = sql_event->query();
  Stmt *stmt = nullptr;
  RC rc = Stmt::create_stmt(db, *query, stmt);
  if (rc != RC::SUCCESS && rc != RC::UNIMPLENMENT) {
    LOG_WARN("failed to create stmt. rc=%d:%s", rc, strrc(rc));
    session_event->set_response("FAILURE\n");
    return;
  }

  sql_event->set_stmt(stmt);

  query_cache_stage_->handle_event(sql_event);

  LOG_TRACE("Exit\n");
  return;
}

void ResolveStage::callback_event(StageEvent *event, CallbackContext *context)
{
  LOG_TRACE("Enter\n");

  LOG_TRACE("Exit\n");
  return;
}
