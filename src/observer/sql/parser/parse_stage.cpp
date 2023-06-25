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
  std::list<Stage *>::iterator stgp = next_stage_list_.begin();
  // optimize_stage_ = *(stgp++);
  resolve_stage_ = *(stgp++);

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
  RC rc = handle_request(event);
  if (RC::SUCCESS != rc) {
    LOG_ERROR("Failed to handle request");
  }

  SQLStageEvent *sql_event = static_cast<SQLStageEvent *>(event);
  sql_event->session_event()->done_immediate();
  sql_event->done_immediate();
  return;
}

RC ParseStage::handle_request(StageEvent *event)
{
  SQLStageEvent *sql_event = static_cast<SQLStageEvent *>(event);
  SqlResult *sql_result = sql_event->session_event()->sql_result();
  const std::string &sql = sql_event->sql();

  ParsedSqlResult parsed_sql_result;

  parse(sql.c_str(), &parsed_sql_result);
  if (parsed_sql_result.commands().empty()) {
    sql_result->set_return_code(RC::SUCCESS);
    sql_result->set_state_string("");
    return RC::INTERNAL;
  }

  if (parsed_sql_result.commands().size() > 1) {
    LOG_WARN("got multi sql commands but only 1 will be handled");
  }

  std::unique_ptr<Command> cmd = std::move(parsed_sql_result.commands().front());
  if (cmd->flag == SCF_ERROR) {
    // set error information to event
    sql_result->set_return_code(RC::SQL_SYNTAX);
    sql_result->set_state_string("Failed to parse sql");
    return RC::INTERNAL;
  }

  sql_event->set_command(std::move(cmd));

  resolve_stage_->handle_event(event);
  return RC::SUCCESS;
}
