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
// Created by Longda on 2010
//

// Include Files
#include "common/seda/stage_event.h"

#include <assert.h>
#include <stdlib.h>

#include "common/defs.h"
#include "common/lang/mutex.h"
#include "common/log/log.h"
#include "common/seda/callback.h"
#include "common/time/timeout_info.h"
namespace common {

// Constructor
StageEvent::StageEvent()
  : comp_cb_(NULL), ud_(NULL), cb_flag_(false), history_(NULL), stage_hops_(0),
    tm_info_(NULL) {}

// Destructor
StageEvent::~StageEvent() {
  // clear all pending callbacks
  while (comp_cb_) {
    CompletionCallback *top = comp_cb_;
    comp_cb_ = comp_cb_->pop_callback();
    delete top;
  }

  // delete the history_ if present
  if (history_) {
    history_->clear();
    delete history_;
  }

  if (tm_info_) {
    tm_info_->detach();
    tm_info_ = NULL;
  }
}

// Processing for this event is done; callbacks executed
void StageEvent::done() {
  CompletionCallback *top;

  if (comp_cb_) {
    top = comp_cb_;
    mark_callback();
    top->event_reschedule(this);
  } else {
    delete this;
  }
}

// Processing for this event is done; callbacks executed immediately
void StageEvent::done_immediate() {
  CompletionCallback *top;

  if (comp_cb_) {
    top = comp_cb_;
    clear_callback();
    comp_cb_ = comp_cb_->pop_callback();
    top->event_done(this);
    delete top;
  } else {
    delete this;
  }
}

void StageEvent::done_timeout() {
  CompletionCallback *top;

  if (comp_cb_) {
    top = comp_cb_;
    clear_callback();
    comp_cb_ = comp_cb_->pop_callback();
    top->event_timeout(this);
    delete top;
  } else {
    delete this;
  }
}

// Push the completion callback onto the stack
void StageEvent::push_callback(CompletionCallback *cb) {
  cb->push_callback(comp_cb_);
  comp_cb_ = cb;
}

void StageEvent::set_user_data(UserData *u) {
  ud_ = u;
  return;
}

UserData *StageEvent::get_user_data() { return ud_; }

// Add stage to list of stages which have handled this event
void StageEvent::save_stage(Stage *stg, HistType type) {
  if (!history_) {
    history_ = new std::list<HistEntry>;
  }
  if (history_) {
    history_->push_back(std::make_pair(stg, type));
    stage_hops_++;
    ASSERT(stage_hops_ <= get_max_event_hops(), "Event exceeded max hops");
  }
}

void StageEvent::set_timeout_info(time_t deadline) {
  TimeoutInfo *tmi = new TimeoutInfo(deadline);
  set_timeout_info(tmi);
}

void StageEvent::set_timeout_info(const StageEvent &ev) {
  set_timeout_info(ev.tm_info_);
}

void StageEvent::set_timeout_info(TimeoutInfo *tmi) {
  // release the previous timeout info
  if (tm_info_) {
    tm_info_->detach();
  }

  tm_info_ = tmi;
  if (tm_info_) {
    tm_info_->attach();
  }
}

bool StageEvent::has_timed_out() {
  if (!tm_info_) {
    return false;
  }

  return tm_info_->has_timed_out();
}

// Accessor function which wraps value for max hops an event is allowed
u32_t &get_max_event_hops() {
  static u32_t max_event_hops = 0;
  return max_event_hops;
}

// Accessor function which wraps value for event history flag
bool &get_event_history_flag() {
  static bool event_history_flag = false;
  return event_history_flag;
}

} //namespace common