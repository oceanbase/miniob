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

#pragma once

// Include Files
#include "common/defs.h"
namespace common {

class StageEvent;
class Stage;
class CallbackContext;

/**
 * A generic CompletionCallback
 * A completion callback object provides a function that should be
 * invoked when an event has made it successfully through the stage
 * pipeline.  Usually, each event will reference a completion callback,
 * and before an event is deleted, the stage doing the deletion should
 * invoke the "done()" event method.  This method eventually invokes the
 * "callback_event()" method in the stage which set the callback, providing
 * a reference to the event as a parameter.
 * <p>
 * The purpose of the callback is to allow a stage to register some
 * processing for an event that is delayed until after the event has
 * progressed all the way through the stage pipeline.  Callbacks can be
 * chained.  Typically, each stage in the pipeline might add a callback
 * to an event's callback chain before passing the event to the next stage.
 * When the "done()" method is finally invoked, the callback on top of the
 * callback stack is invoked.  It becomes the responsibility of this callback
 * to either forward the event to another stage for more processing, or
 * to eventually call done() again of the event.  In this way, with each
 * callback on the stack eventually invoking the next callback on the stack,
 * all the callbacks are eventually executed. Each callback
 * can have an optional context associated with it.  This context is
 * provided to the stage callback function when it is invoked.  It is
 * opaque to the callback object.
 * <p>
 * By default, the callback will run on the thread of the stage that created
 * the callback.  If the stage that is calling done() on the event wants
 * to execute the callback stack in place, it can call the done_immediate()
 * interface.  Note that this will execute the *entire* callback stack on
 * the current thread.
 */

class CompletionCallback {

  // public interface operations

public:
  // Constructor
  CompletionCallback(Stage *trgt, CallbackContext *ctx = NULL);

  // Destructor
  virtual ~CompletionCallback();

  // Push onto a callback stack
  void push_callback(CompletionCallback *stack);

  /**
   * Pop off of a callback stack
   * @returns  remainder of callback stack
   */
  CompletionCallback *pop_callback();

  // One event is complete
  void event_done(StageEvent *ev);

  // Reschedule this event as a callback on the target stage
  void event_reschedule(StageEvent *ev);

  // Complete this event if it has timed out
  void event_timeout(StageEvent *ev);

protected:
  // implementation state

  Stage *target_stage_;          // stage which is setting this callback
  CallbackContext *context_;     // argument to pass when invoking cb
  CompletionCallback *next_cb_;  // next event in the chain
  bool ev_hist_flag_;            // true if event histories are enabled
};

/**
 *  Context attached to callback
 *  The callback context may be optionally supplied to a callback.  It
 *  is useful for passing extra arguments to the callback function when
 *  invoked.  To make use of this feature, a stage should derive its own
 *  callback context class from this base.
 */
class CallbackContext {
public:
  virtual ~CallbackContext()
  {}
};

class CallbackContextEvent : public CallbackContext {
public:
  CallbackContextEvent(StageEvent *event = NULL) : ev_(event)
  {}
  ~CallbackContextEvent()
  {}
  StageEvent *get_event()
  {
    return ev_;
  }

private:
  StageEvent *ev_;
};

}  // namespace common
