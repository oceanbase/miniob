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
// Created by Longda on 2010
//

// Include Files
#include "common/seda/callback.h"

#include <assert.h>

#include "common/log/log.h"
#include "common/seda/stage.h"
#include "common/seda/stage_event.h"
namespace common {

extern bool &get_event_history_flag();

/**
 * @author Longda
 * @date   3/27/07
 *
 * Implementation of CompletionCallback class.
 */

// Constructor
CompletionCallback::CompletionCallback(Stage *trgt, CallbackContext *ctx)
    : target_stage_(trgt), context_(ctx), next_cb_(NULL), ev_hist_flag_(get_event_history_flag())
{}

// Destructor
CompletionCallback::~CompletionCallback()
{
  if (context_) {
    delete context_;
  }
  if (next_cb_) {
    delete next_cb_;
  }
}

// Push onto a callback stack
void CompletionCallback::push_callback(CompletionCallback *next)
{
  ASSERT((!next_cb_), "%s", "cannot push a callback twice");

  next_cb_ = next;
}

// Pop off of a callback stack
CompletionCallback *CompletionCallback::pop_callback()
{
  CompletionCallback *ret_val = next_cb_;

  next_cb_ = NULL;
  return ret_val;
}

// One event is complete
void CompletionCallback::event_done(StageEvent *ev)
{

  if (ev_hist_flag_) {
    ev->save_stage(target_stage_, StageEvent::CALLBACK_EV);
  }
  target_stage_->callback_event(ev, context_);
}

// Reschedule callback on target stage thread
void CompletionCallback::event_reschedule(StageEvent *ev)
{
  target_stage_->add_event(ev);
}

void CompletionCallback::event_timeout(StageEvent *ev)
{
  LOG_DEBUG("to call event_timeout for stage %s", target_stage_->get_name());
  if (ev_hist_flag_) {
    ev->save_stage(target_stage_, StageEvent::TIMEOUT_EV);
  }
  target_stage_->timeout_event(ev, context_);
}

}  // namespace common