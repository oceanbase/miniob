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

#include "example_stage.h"

#include <string.h>
#include <string>

#include "common/conf/ini.h"
#include "common/io/io.h"
#include "common/lang/string.h"
#include "common/log/log.h"
#include "common/seda/timer_stage.h"

using namespace common;

// Constructor
ExampleStage::ExampleStage(const char *tag) : Stage(tag)
{}

// Destructor
ExampleStage::~ExampleStage()
{}

// Parse properties, instantiate a stage object
Stage *ExampleStage::make_stage(const std::string &tag)
{
  ExampleStage *stage = new ExampleStage(tag.c_str());
  if (stage == NULL) {
    LOG_ERROR("new ExampleStage failed");
    return NULL;
  }
  stage->set_properties();
  return stage;
}

// Set properties for this object set in stage specific properties
bool ExampleStage::set_properties()
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

// Initialize stage params and validate outputs
bool ExampleStage::initialize()
{
  LOG_TRACE("Enter");

  //  std::list<Stage*>::iterator stgp = next_stage_list_.begin();
  //  mTimerStage = *(stgp++);
  //  mCommStage = *(stgp++);

  LOG_TRACE("Exit");
  return true;
}

// Cleanup after disconnection
void ExampleStage::cleanup()
{
  LOG_TRACE("Enter");

  LOG_TRACE("Exit");
}

void ExampleStage::handle_event(StageEvent *event)
{
  LOG_TRACE("Enter\n");

  LOG_TRACE("Exit\n");
  return;
}

void ExampleStage::callback_event(StageEvent *event, CallbackContext *context)
{
  LOG_TRACE("Enter\n");

  LOG_TRACE("Exit\n");
  return;
}
