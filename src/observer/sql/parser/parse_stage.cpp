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

#include "parse_stage.h"

#include "common/conf/ini.h"
#include "common/io/io.h"
#include "common/lang/string.h"
#include "common/log/log.h"
#include "common/seda/timer_stage.h"
#include "event/session_event.h"
#include "event/sql_event.h"
#include "sql/parser/parse.h"
#include "event/execution_plan_event.h"

using namespace common;

//! Constructor
ParseStage::ParseStage(const char *tag) : Stage(tag)
{}

//! Destructor
ParseStage::~ParseStage()
{}

//! Parse properties, instantiate a stage object
Stage *ParseStage::make_stage(const std::string &tag)
{
  ParseStage *stage = new (std::nothrow) ParseStage(tag.c_str());
  if (stage == nullptr) {
    LOG_ERROR("new ParseStage failed");
    return nullptr;
  }
  stage->set_properties();
  return stage;
}

//! Set properties for this object set in stage specific properties
bool ParseStage::set_properties()
{
  //  std::string stageNameStr(stageName);
  //  std::map<std::string, std::string> section = theGlobalProperties()->get(
  //    stageNameStr);
  //
  //  std::map<std::string, std::string>::iterator it;
  //
  //  std::string key;

  return true;
}

//! Initialize stage params and validate outputs
bool ParseStage::initialize()
{
  LOG_TRACE("Enter");

  std::list<Stage *>::iterator stgp = next_stage_list_.begin();
  optimize_stage_ = *(stgp++);

  LOG_TRACE("Exit");
  return true;
}

//! Cleanup after disconnection
void ParseStage::cleanup()
{
  LOG_TRACE("Enter");

  LOG_TRACE("Exit");
}

void ParseStage::handle_event(StageEvent *event)
{
  LOG_TRACE("Enter\n");

  StageEvent *new_event = handle_request(event);
  if (nullptr == new_event) {
    callback_event(event, nullptr);
    event->done_immediate();
    return;
  }

  CompletionCallback *cb = new (std::nothrow) CompletionCallback(this, nullptr);
  if (cb == nullptr) {
    LOG_ERROR("Failed to new callback for SQLStageEvent");
    callback_event(event, nullptr);
    event->done_immediate();
    return;
  }
  event->push_callback(cb);
  optimize_stage_->handle_event(new_event);

  LOG_TRACE("Exit\n");
  return;
}

void ParseStage::callback_event(StageEvent *event, CallbackContext *context)
{
  LOG_TRACE("Enter\n");
  SQLStageEvent *sql_event = static_cast<SQLStageEvent *>(event);
  sql_event->session_event()->done_immediate();
  LOG_TRACE("Exit\n");
  return;
}

StageEvent *ParseStage::handle_request(StageEvent *event)
{
  SQLStageEvent *sql_event = static_cast<SQLStageEvent *>(event);
  const std::string &sql = sql_event->get_sql();

  Query *result = query_create();
  if (nullptr == result) {
    LOG_ERROR("Failed to create query.");
    return nullptr;
  }

  RC ret = parse(sql.c_str(), result);
  if (ret != RC::SUCCESS) {
    // set error information to event
    const char *error = result->sstr.errors != nullptr ? result->sstr.errors : "Unknown error";
    char response[256];
    snprintf(response, sizeof(response), "Failed to parse sql: %s, error msg: %s\n", sql.c_str(), error);
    sql_event->session_event()->set_response(response);
    query_destroy(result);
    return nullptr;
  }

  return new ExecutionPlanEvent(sql_event, result);
}
