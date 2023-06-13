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

#include "common/seda/metrics_stage.h"

#include <string.h>
#include <string>

#include "common/conf/ini.h"
#include "common/io/io.h"
#include "common/lang/string.h"
#include "common/log/log.h"
#include "common/metrics/metrics_registry.h"
#include "common/seda/metrics_report_event.h"
#include "common/seda/timer_stage.h"
#include "common/time/datetime.h"
#include "common/seda/seda_defs.h"

using namespace common;

MetricsRegistry &get_metric_registry()
{
  static MetricsRegistry metrics_registry;

  return metrics_registry;
}

// Constructor
MetricsStage::MetricsStage(const char *tag) : Stage(tag)
{}

// Destructor
MetricsStage::~MetricsStage()
{}

// Parse properties, instantiate a stage object
Stage *MetricsStage::make_stage(const std::string &tag)
{
  MetricsStage *stage = new MetricsStage(tag.c_str());
  if (stage == NULL) {
    LOG_ERROR("new MetricsStage failed");
    return NULL;
  }
  stage->set_properties();
  return stage;
}

// Set properties for this object set in stage specific properties
bool MetricsStage::set_properties()
{
  std::string stage_name_str(stage_name_);
  std::map<std::string, std::string> section = get_properties()->get(stage_name_str);

  metric_report_interval_ = DateTime::SECONDS_PER_MIN;

  std::string key = METRCS_REPORT_INTERVAL;
  std::map<std::string, std::string>::iterator it = section.find(key);
  if (it != section.end()) {
    str_to_val(it->second, metric_report_interval_);
  }

  return true;
}

// Initialize stage params and validate outputs
bool MetricsStage::initialize()
{
  std::list<Stage *>::iterator stgp = next_stage_list_.begin();
  timer_stage_ = *(stgp++);

  MetricsReportEvent *report_event = new MetricsReportEvent();

  add_event(report_event);
  return true;
}

// Cleanup after disconnection
void MetricsStage::cleanup()
{
}

void MetricsStage::handle_event(StageEvent *event)
{
  CompletionCallback *cb = new CompletionCallback(this, NULL);
  if (cb == NULL) {
    LOG_ERROR("Failed to new callback");

    event->done();

    return;
  }

  TimerRegisterEvent *tm_event = new TimerRegisterEvent(event, metric_report_interval_ * USEC_PER_SEC);
  if (tm_event == NULL) {
    LOG_ERROR("Failed to new TimerRegisterEvent");

    delete cb;

    event->done();

    return;
  }

  event->push_callback(cb);
  timer_stage_->add_event(tm_event);

  return;
}

void MetricsStage::callback_event(StageEvent *event, CallbackContext *context)
{
  MetricsRegistry &metrics_registry = get_metrics_registry();

  metrics_registry.snapshot();
  metrics_registry.report();

  // do it again.
  add_event(event);

  return;
}
