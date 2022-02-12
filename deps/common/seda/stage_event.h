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
#ifndef __COMMON_SEDA_STAGE_EVENT_H__
#define __COMMON_SEDA_STAGE_EVENT_H__

// Include Files
#include <time.h>
#include <list>
#include <map>
#include <string>

#include "common/defs.h"
namespace common {

class CompletionCallback;
class UserData;
class Stage;
class TimeoutInfo;

//! An event in a staged event-driven architecture
/**
 * Abstract base class for all events.  Each event can reference
 * a stack of completion callbacks. A callback is added to an event using
 * the push_callback() method.  The first completion callback on the stack
 * is invoked by calling the done() method of StageEvent.  The Stage code
 * handling an event (either in handle_event() or callback_event()) has
 * certain responsibilities regarding how the event and its associated
 * callbacks are processed. When a stage finishes processing an
 * event it has the following options:
 * <ul>
 * <li>Pass the event along to another stage for processing.  In this
 * case the responsibility to eventually call done() passes to the next
 * stage.
 * <li>Pass the event along to another stage for processing but add a
 * callback to the event for the current stage.  In this case, the stage
 * must create a callback object and use the push_callback() interface
 * to add the callback to the top of the event's callback stack. Again,
 * the responsibility for calling done() passes to the next stage.
 * <li>Dispose of the event.  This is achieved by calling done().  After
 * calling done() the stage must not access the event again.  Note that
 * done() will result in callbacks attached to the event being executed
 * asynchronously by the threadpool of the stage which set the callback.
 * Calling done_immediate() has the same effect as done(), except that the
 * callbacks are executed on the current stack.
 * </ul>
 */

class StageEvent {

public:
  // Interface for collecting debugging information
  typedef enum { HANDLE_EV = 0, CALLBACK_EV, TIMEOUT_EV } HistType;

  /**
   *  Constructor
   *  Should not create StageEvents on the stack.  done() assumes that
   *  event is dynamically allocated.
   */
  StageEvent();

  /**
   *  Destructor
   *  Should only be called from done(), or from Stage class
   *  during cleanup.  Public for now, because constructor is public.
   */
  virtual ~StageEvent();

  // Processing for this event is done; execute callbacks
  // this will trigger thread switch if there are callbacks,  this will be async
  // Calling done_immediate won't trigger thread switch, this will be synchonized
  void done();

  // Processing for this event is done; execute callbacks immediately
  // Calling done_immediate won't trigger thread switch, this will be synchonized
  void done_immediate();

  /**
   *  Processing for this event is done if the event has timed out
   *  \c timeout_event() will be called instead of \c callback_event()
   *  if the event has timed out.
   */
  void done_timeout();

  // Set the completion callback
  void push_callback(CompletionCallback *cb);

  /**
   *  Set the originating event that caused this event.
   *  The caller is responsible for recovering the memory associated with
   *  the \c UserData object.
   */
  void set_user_data(UserData *u);

  // Get the originating event the caused this event.
  UserData *get_user_data();

  // True if event represents a callback
  bool is_callback()
  {
    return cb_flag_;
  }

  // Add stage to list of stages which have handled this event
  void save_stage(Stage *stg, HistType type);

  /**
   * Set a timeout info into the event
   * @param[in] deadline  deadline of the timeout
   */
  void set_timeout_info(time_t deadline);

  // Share a timeout info with another \c StageEvent
  void set_timeout_info(const StageEvent &ev);

  // If the event has timed out (and should be dropped)
  bool has_timed_out();

private:
  typedef std::pair<Stage *, HistType> HistEntry;

  // Interface to allow callbacks to be run on target stage's threads
  void mark_callback()
  {
    cb_flag_ = true;
  }
  void clear_callback()
  {
    cb_flag_ = false;
  }

  // Set a timeout info into the event
  void set_timeout_info(TimeoutInfo *tmi);

  CompletionCallback *comp_cb_;    // completion callback stack for this event
  UserData *ud_;                   // user data associated with event by caller
  bool cb_flag_;                   // true if this event is a callback
  std::list<HistEntry> *history_;  // List of stages which have handled ev
  u32_t stage_hops_;               // Number of stages which have handled ev
  TimeoutInfo *tm_info_;           // the timeout info for this event
};

/**
 *
 *  \brief An opaque data structure that can be associated with any event to
 *  maintain the state of the calling stage.
 *
 *  Setting the \c UserData member is optional.  The caller may elect to set
 *  the \c UserData member to maintain state across a request to another
 *  stage.  If the \c UserData member is used, the calling state should create
 *  a new class derived from \c UserData to store state relevant to its
 *  processing.  When the called stage invokes the \c CompletionCallback, the
 *  originating stage can access the \c UserData member to recover its state.
 */
class UserData {
public:
  /**
   *  \brief A virtual destructor to enable the use of dynamic casts.
   */
  virtual ~UserData()
  {
    return;
  }
};

bool &get_event_history_flag();
u32_t &get_max_event_hops();

}  // namespace common
#endif  // __COMMON_SEDA_STAGE_EVENT_H__
