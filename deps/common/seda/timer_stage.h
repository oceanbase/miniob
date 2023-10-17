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

#include <pthread.h>
#include <sys/time.h>
#include <memory>

#include "common/log/log.h"
#include "common/seda/callback.h"
#include "common/seda/stage.h"
#include "common/seda/stage_event.h"
namespace common {

#define NSEC_PER_SEC 1000000000
#define USEC_PER_SEC 1000000
#define NSEC_PER_USEC 1000

/**
 *  \author longda
 *  \date October 22, 2007
 *
 *  \brief A token that can be used to cancel an event that has been
 *  registered with the \c TimerStage.
 *
 *  A token uniquely identifies events managed by the \c TimerStage.
 *  It should be considered opaque to all components other than the \c
 *  TimerStage.
 */
class TimerToken {
public:
  TimerToken();
  TimerToken(const struct timeval &t);
  TimerToken(const TimerToken &tt);
  const struct timeval &get_time() const;
  uint64_t get_nonce() const;
  bool operator<(const TimerToken &other) const;
  TimerToken &operator=(const TimerToken &src);
  std::string to_string() const;

  friend bool timer_token_less_than(const TimerToken &tt1, const TimerToken &tt2);

private:
  void set(const struct timeval &t, uint64_t n);
  static uint64_t next_nonce();

  struct timeval time;
  uint64_t nonce;
};

/**
 *  \author Longda
 *  \date October 22, 2007
 *
 *  \brief An abstract base class for all timer-related events.
 */
class TimerEvent : public StageEvent {
public:
  TimerEvent() : StageEvent()
  {
    return;
  }
  virtual ~TimerEvent()
  {
    return;
  }
};

/**
 *  \author Longda
 *  \date October 22, 2007
 *
 *  \brief An event requesting to register a timer callback with the
 *  \c TimerStage.
 *
 *  This request includes an event to trigger and a time (specified in
 *  either relative time or absolute time) when the event should
 *  trigger.  The event will never be triggered before the requested
 *  time.  Depending on factors such as the load on the system and the
 *  number of available threads, the event may be triggered later than
 *  the requested time.
 */
class TimerRegisterEvent : public TimerEvent {
public:
  /**
   *  \brief Create an event to request the registration of a timer
   *  callback using relative time.
   *
   *  \arg cb
   *    The event to trigger when the timer fires.
   *  \arg time_relative_usec
   *    The amount of time (in microseconds) before the timer
   *    triggering the callback should fire.
   */
  TimerRegisterEvent(StageEvent *cb, uint64_t time_relative_usec);

  /**
   *  \brief Create an event to request the registration of a timer
   *  callback using absolute time.
   *
   *  \arg cb
   *    The event to trigger when the timer fires.
   *  \arg time_absolute
   *    The absolute time when the timer triggering the callback
   *    should fire.
   *
   *  Notes:
   *  Change system time will affect CLOCK_REALTIME and may
   *  affect timers.
   */
  TimerRegisterEvent(StageEvent *cb, struct timeval &time_absolute);

  /**
   *  \brief Destroy the event.
   */
  ~TimerRegisterEvent();

  /**
   *  \brief Get the opaque token that can be used later to cancel
   *  the timer callback created by this event.
   *
   *  This function will copy the token and then return a pointer to
   *  the token.  The caller should treat the value that is returned
   *  as opaque.  The copy of the token will be destroyed
   *  automatically when the pointer to the token falls out of
   *  scope.
   *
   *  The result of this function is undefined if it is called
   *  before the \c TimerStage has handled the event.
   *
   *  \return
   *    A pointer to a token that can be used to cancel the timer
   *    callback that was created by this request.
   */
  std::unique_ptr<const TimerToken> get_cancel_token();

  /**
   *  \brief Get the absolute time when the callback is to be triggered.
   *
   *  \return
   *    The absolute time when the callback is to be triggered.
   */
  const struct timeval &get_time();

  /**
   *  \brief Get the callback event to be triggered by the timer.
   *
   *  \return
   *    The callback event to be invoked after the timer fires.
   */
  StageEvent *get_callback_event();

  /**
   *  \brief Assume responsiblity for management of callback event.
   *
   *  The \c TimerStage will use this method to dissociate the
   *  callback event from the register event.  The \c TimerStage
   *  will then hold the callback event until the appropriate timer
   *  fires.
   */
  StageEvent *adopt_callback_event();

  /**
   *  \brief Assign the token that can be used to cancel the timer
   *  event.
   *
   *  \arg t
   *    The opaque token that the caller can use to later cancel the
   *    callback event.
   */
  void set_cancel_token(const TimerToken &t);

private:
  StageEvent *timer_cb_;
  struct timeval timer_when_;
  TimerToken token_;
};

/**
 *
 *  \brief An event requesting to cancel a timer callback that was
 *  previously registered with the \c TimerStage.
 *
 *  The success of the cancellation request is not assured.  The
 *  success of the request will depend on the relative ordering of
 *  events in the system.  The \c TimerStage will report whether the
 *  event was cancelled successfully or not.  If the \c TimerStage
 *  reports that the event is cancelled, it will not invoke trigger
 *  the associated callback event.
 */
class TimerCancelEvent : public TimerEvent {
public:
  /**
   *  \brief Create an event to request the cancellation of a timer
   *  callback that was previously set.
   *
   *  \arg cancel_token
   *    A pointer to the opaque token (obtained from \c
   *    TimerRegisterEvent.get_cancel_token()) that identifies the
   *    timer callback to be cancelled.
   */
  TimerCancelEvent(const TimerToken &cancel_token);

  /**
   *  \brief Destroy the event.
   */
  ~TimerCancelEvent();

  /**
   *  \brief Report whether the event was successfully cancelled.
   *
   *  \return
   *    \c true if the event was cancelled before it was triggered;
   *    \c false otherwise
   */
  bool get_success();

  /**
   *  \brief Set the status reporting whether the event was
   *  successfully cancelled.
   *
   *  \arg s
   *    \c true if the event was successfully cancelled; \c false
   *    otherwise
   */
  void set_success(bool s);

  /**
   *  \brief Get the token corresponding to the event to be cancelled.
   */
  const TimerToken &get_token();

private:
  TimerToken token_;
  bool cancelled_;
};

/**
 *
 *  \brief A stage that triggers event callbacks at some future time.
 *
 *  The \c TimerStage is designed to trigger event callbacks at some
 *  time in the future, as requested by the user of the \c TimerStage.
 *
 *  To request that an event be triggered in the future, the user
 *  should submit a \c TimerRegisterEvent that include the event to be
 *  triggered and the time (in either absolute or relative form) when
 *  it should be triggered.  If the user sets the callback of the \c
 *  TimerRegisterEvent, the \c TimerStage will return to the user a
 *  token by which the registration can be cancelled.
 *
 *  To cancel a registered timer event, the user should submit a \c
 *  TimerCancelEvent that includes the token returned to the user in
 *  the previous \c TimerRegisterEvent.  If the request to cancel the
 *  event reaches the \c TimerStage before the event is triggered, the
 *  \c TimerStage will cancel the event.  The \c TimerStage will
 *  report, via the callback of the \c TimerCancelEvent, whether the
 *  event was successfully cancelled.
 *
 *  The \c TimerStage will invoke the callback of an event when the
 *  time requested by \c TimerRegisterEvent is reached.  A callback
 *  will never be invoked before the requested time; the punctuality
 *  of the event triggering will depend on the load on the system.
 *
 *  Implementation note: The \c TimerStage creates an internal thread
 *  to maintain the timer.
 */
class TimerStage : public Stage {
public:
  ~TimerStage();
  static Stage *make_stage(const std::string &tag);

  /**
   *  \brief Return the number of events that have been registered
   *  but not yet triggered or cancelled.
   */
  uint32_t get_num_events();

protected:
  TimerStage(const char *tag);
  bool set_properties();
  bool initialize();
  void handle_event(StageEvent *event);
  void callback_event(StageEvent *event, CallbackContext *context);
  void disconnect_prepare();

  // For ordering the keys in the timer_queue_.
  static bool timer_token_less_than(const TimerToken &tt1, const TimerToken &tt2);

private:
  void register_timer(TimerRegisterEvent &reg_ev);
  void cancel_timer(TimerCancelEvent &cancel_ev);
  bool timeval_less_than(const struct timeval &t1, const struct timeval &t2);
  void trigger_timer_check();
  void check_timer();

  static void *start_timer_thread(void *arg);

  typedef std::map<TimerToken, StageEvent *, bool (*)(const TimerToken &, const TimerToken &)> timer_queue_t;
  timer_queue_t timer_queue_;

  pthread_mutex_t timer_mutex_;
  pthread_cond_t timer_condv_;

  bool shutdown_;              // true if stage has received the shutdown signal
  uint32_t num_events_;           // the number of timer events currently outstanding
  pthread_t timer_thread_id_;  // thread id of the timer maintenance thread
};

}  // namespace common

