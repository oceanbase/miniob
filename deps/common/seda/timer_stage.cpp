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

#include "common/seda/timer_stage.h"

#include <string.h>
#include <time.h>

#include <iostream>
#include <memory>
#include <sstream>
#include <typeinfo>

#include "common/lang/mutex.h"
#include "common/log/log.h"
namespace common {

#define TIMEVAL_EQUAL(t1, t2) ((t1.tv_sec == t2.tv_sec) && (t1.tv_usec == t2.tv_usec))
#define TIMEVAL_LESS_THAN(t1, t2) ((t1.tv_sec < t2.tv_sec) || ((t1.tv_sec == t2.tv_sec) && (t1.tv_usec < t2.tv_usec)))

struct timeval sub_timeval(const struct timeval *t1, const struct timeval *t2)
{
  struct timeval result;
  result.tv_sec = t1->tv_sec - t2->tv_sec;
  result.tv_usec = t1->tv_usec - t2->tv_usec;
  while (result.tv_usec < 0) {
    --result.tv_sec;
    result.tv_usec += USEC_PER_SEC;
  }
  return result;
}

struct timeval add_timeval(const struct timeval *t1, const struct timeval *t2)
{
  struct timeval result;
  result.tv_sec = t1->tv_sec + t2->tv_sec;
  result.tv_usec = t1->tv_usec + t2->tv_usec;
  while (result.tv_usec >= USEC_PER_SEC) {
    ++result.tv_sec;
    result.tv_usec -= USEC_PER_SEC;
  }
  return result;
}

void realtime_to_monotonic(const struct timeval *time_RT, struct timeval *time_Mono)
{

  struct timeval time_now;
  gettimeofday(&time_now, NULL);
  struct timeval time_offset;
  time_offset = sub_timeval(time_RT, &time_now);

  struct timespec time_ts;
  clock_gettime(CLOCK_MONOTONIC, &time_ts);

  struct timeval time_temp;
  time_temp.tv_sec = time_ts.tv_sec;
  time_temp.tv_usec = time_ts.tv_nsec / NSEC_PER_USEC;
  time_temp = add_timeval(&time_temp, &time_offset);

  time_Mono->tv_sec = time_temp.tv_sec;
  time_Mono->tv_usec = time_temp.tv_usec;
}

u64_t TimerToken::next_nonce()
{
  static u64_t nonce_cntr = 0;
  static pthread_mutex_t tt_mutex = PTHREAD_MUTEX_INITIALIZER;

  pthread_mutex_lock(&tt_mutex);
  u64_t n = nonce_cntr++;
  pthread_mutex_unlock(&tt_mutex);

  return n;
}

TimerToken::TimerToken()
{
  struct timeval t;
  memset(&t, 0, sizeof(struct timeval));
  u64_t n = next_nonce();
  set(t, n);
  return;
}

TimerToken::TimerToken(const struct timeval &t)
{
  u64_t n = next_nonce();
  set(t, n);
  return;
}

TimerToken::TimerToken(const TimerToken &tt)
{
  set(tt.time, tt.nonce);
  return;
}

void TimerToken::set(const struct timeval &t, u64_t n)
{
  memcpy(&time, &t, sizeof(struct timeval));
  nonce = n;
  return;
}

const struct timeval &TimerToken::get_time() const
{
  return time;
}

u64_t TimerToken::get_nonce() const
{
  return nonce;
}

bool TimerToken::operator<(const TimerToken &other) const
{
  if (TIMEVAL_LESS_THAN(time, other.time))
    return true;
  if (TIMEVAL_EQUAL(time, other.time))
    return (nonce < other.nonce);
  return false;
}

TimerToken &TimerToken::operator=(const TimerToken &src)
{
  set(src.time, src.nonce);
  return *this;
}

std::string TimerToken::to_string() const
{
  std::string s;
  std::ostringstream ss(s);
  ss << time.tv_sec << ":" << time.tv_usec << "-" << nonce;
  return ss.str();
}

TimerRegisterEvent::TimerRegisterEvent(StageEvent *cb, u64_t time_relative_usec) : TimerEvent(), timer_cb_(cb), token_()
{
  struct timespec timer_spec;
  clock_gettime(CLOCK_MONOTONIC, &timer_spec);

  timer_when_.tv_sec = timer_spec.tv_sec;
  timer_when_.tv_usec = (timer_spec.tv_nsec / NSEC_PER_USEC);

  timer_when_.tv_usec += time_relative_usec;
  if (timer_when_.tv_usec >= USEC_PER_SEC) {
    timer_when_.tv_sec += (timer_when_.tv_usec / USEC_PER_SEC);
    timer_when_.tv_usec = (timer_when_.tv_usec % USEC_PER_SEC);
  }

  return;
}

TimerRegisterEvent::TimerRegisterEvent(StageEvent *cb, struct timeval &time_absolute)
    : TimerEvent(), timer_cb_(cb), token_()
{
  realtime_to_monotonic(&time_absolute, &timer_when_);
  return;
}

TimerRegisterEvent::~TimerRegisterEvent()
{
  return;
}

const struct timeval &TimerRegisterEvent::get_time()
{
  return timer_when_;
}

StageEvent *TimerRegisterEvent::get_callback_event()
{
  return timer_cb_;
}

StageEvent *TimerRegisterEvent::adopt_callback_event()
{
  StageEvent *e = timer_cb_;
  timer_cb_ = NULL;
  return e;
}

void TimerRegisterEvent::set_cancel_token(const TimerToken &t)
{
  token_ = t;
  return;
}

std::unique_ptr<const TimerToken> TimerRegisterEvent::get_cancel_token()
{
  const TimerToken *token_cp = new TimerToken(token_);
  std::unique_ptr<const TimerToken> token_ptr(token_cp);
  return token_ptr;
}

TimerCancelEvent::TimerCancelEvent(const TimerToken &cancel_token)
    : TimerEvent(), token_(cancel_token), cancelled_(false)
{
  return;
}

TimerCancelEvent::~TimerCancelEvent()
{
  return;
}

const TimerToken &TimerCancelEvent::get_token()
{
  return token_;
}

void TimerCancelEvent::set_success(bool s)
{
  cancelled_ = s;
  return;
}

bool TimerCancelEvent::get_success()
{
  return cancelled_;
}

TimerStage::TimerStage(const char *tag)
    : Stage(tag),
      timer_queue_(&TimerStage::timer_token_less_than),
      shutdown_(false),
      num_events_(0),
      timer_thread_id_(0)
{
  pthread_mutex_init(&timer_mutex_, NULL);
  pthread_condattr_t condattr;
  pthread_condattr_init(&condattr);
#ifdef LINUX
  pthread_condattr_setclock(&condattr, CLOCK_MONOTONIC);
#endif
  pthread_cond_init(&timer_condv_, &condattr);
  pthread_condattr_destroy(&condattr);
  return;
}

TimerStage::~TimerStage()
{
  for (timer_queue_t::iterator i = timer_queue_.begin(); i != timer_queue_.end(); ++i) {
    delete i->second;
  }

  num_events_ = 0;

  pthread_mutex_destroy(&timer_mutex_);
  pthread_cond_destroy(&timer_condv_);

  return;
}

Stage *TimerStage::make_stage(const std::string &tag)
{
  TimerStage *s = new TimerStage(tag.c_str());
  ASSERT(s != NULL, "Failed to instantiate stage.");
  if (!s->set_properties()) {
    LOG_PANIC("failed to set properties.\n");
    delete s;
    s = NULL;
  }

  return s;
}

bool TimerStage::set_properties()
{
  // No configuration is stored in the system properties.
  return true;
}

bool TimerStage::initialize()
{
  // The TimerStage does not send messages to any other stage.
  ASSERT(next_stage_list_.size() == 0, "Invalid NextStages list.");

  // Start the thread to maintain the timer
  const pthread_attr_t *thread_attrs = NULL;
  void *thread_args = (void *)this;
  int status = pthread_create(&timer_thread_id_, thread_attrs, &TimerStage::start_timer_thread, thread_args);
  if (status != 0)
    LOG_ERROR("failed to create timer thread: status=%d\n", status);

  return (status == 0);
}

u32_t TimerStage::get_num_events()
{
  return num_events_;
}

void TimerStage::disconnect_prepare()
{
  LOG_INFO("received signal to initiate shutdown_.\n");
  pthread_mutex_lock(&timer_mutex_);
  shutdown_ = true;
  pthread_cond_signal(&timer_condv_);
  pthread_mutex_unlock(&timer_mutex_);

  LOG_TRACE("waiting for timer maintenance thread to terminate.\n");
  void **return_val_ptr = NULL;
  int status;
  status = pthread_join(timer_thread_id_, return_val_ptr);
  LOG_TRACE("timer maintenance thread terminated: status=%d\n", status);

  return;
}

void TimerStage::handle_event(StageEvent *event)
{
  TimerEvent *e = dynamic_cast<TimerEvent *>(event);
  if (e == NULL) {
    LOG_WARN("received event of unexpected type: typeid=%s\n", typeid(*event).name());
    return;  // !!! EARLY EXIT !!!
  }

  TimerRegisterEvent *register_ev = dynamic_cast<TimerRegisterEvent *>(event);
  if (register_ev != NULL) {
    register_timer(*register_ev);
    return;  // !!! EARLY EXIT !!!
  }

  TimerCancelEvent *cancel_ev = dynamic_cast<TimerCancelEvent *>(event);
  if (cancel_ev != NULL) {
    cancel_timer(*cancel_ev);
    return;  // !!! EARLY EXIT !!!
  }

  return;
}

void TimerStage::callback_event(StageEvent *e, CallbackContext *ctx)
{
  return;
}

void TimerStage::register_timer(TimerRegisterEvent &reg_ev)
{
  const TimerToken tt(reg_ev.get_time());

  LOG_TRACE("registering event: token=%s\n", tt.to_string().c_str());

  bool check_timer = false;
  pthread_mutex_lock(&timer_mutex_);

  // add the event to the timer queue
  StageEvent *timer_cb = reg_ev.adopt_callback_event();
  std::pair<timer_queue_t::iterator, bool> result = timer_queue_.insert(std::make_pair(tt, timer_cb));
  ASSERT(result.second,
      "Internal error--"
      "failed to register timer because token is not unique.");
  ++num_events_;

  // if event was added to the head of queue, schedule a timer check
  if (result.first == timer_queue_.begin())
    check_timer = true;

  pthread_mutex_unlock(&timer_mutex_);

  reg_ev.set_cancel_token(tt);
  reg_ev.done();

  if (check_timer)
    trigger_timer_check();

  return;
}

void TimerStage::cancel_timer(TimerCancelEvent &cancel_ev)
{
  pthread_mutex_lock(&timer_mutex_);
  bool success = false;
  timer_queue_t::iterator it = timer_queue_.find(cancel_ev.get_token());
  if (it != timer_queue_.end()) {
    success = true;

    // delete the canceled timer event
    delete it->second;

    timer_queue_.erase(it);
    --num_events_;
  }
  pthread_mutex_unlock(&timer_mutex_);

  LOG_DEBUG("cancelling event: token=%s, success=%d\n", cancel_ev.get_token().to_string().c_str(), (int)success);

  cancel_ev.set_success(success);
  cancel_ev.done();

  return;
}

void TimerStage::trigger_timer_check()
{
  LOG_TRACE("signaling timer thread to complete timer check\n");

  pthread_mutex_lock(&timer_mutex_);
  pthread_cond_signal(&timer_condv_);
  pthread_mutex_unlock(&timer_mutex_);

  return;
}

void *TimerStage::start_timer_thread(void *arg)
{
  TimerStage *tstage = static_cast<TimerStage *>(arg);
  ASSERT(tstage != NULL, "Internal error--failed to start timer thread.");
  tstage->check_timer();
  return NULL;
}

void TimerStage::check_timer()
{
  pthread_mutex_lock(&timer_mutex_);

  while (true) {
    struct timespec ts_now;
    clock_gettime(CLOCK_MONOTONIC, &ts_now);

    struct timeval now;
    now.tv_sec = ts_now.tv_sec;
    now.tv_usec = ts_now.tv_nsec / NSEC_PER_USEC;

    LOG_TRACE("checking timer: sec=%ld, usec=%ld\n", now.tv_sec, now.tv_usec);

    // Trigger all events for which the trigger time has already passed.
    timer_queue_t::iterator first = timer_queue_.begin();
    timer_queue_t::iterator last;
    std::list<StageEvent *> done_events;
    for (last = first; last != timer_queue_.end(); ++last) {
      if (TIMEVAL_LESS_THAN(now, last->first.get_time()))
        break;
      done_events.push_back(last->second);
    }
    timer_queue_.erase(first, last);
    if (!done_events.empty()) {
      // It is ok to hold the mutex while executing this loop.
      // Triggering the events only enqueues the event on the
      // caller's queue--it does not perform any real work.
      for (std::list<StageEvent *>::iterator i = done_events.begin(); i != done_events.end(); ++i) {
        LOG_TRACE(
            "triggering timer event: sec=%ld, usec=%ld, typeid=%s\n", now.tv_sec, now.tv_usec, typeid(**i).name());
        (*i)->done();
        --num_events_;
      }
    }
    done_events.clear();

    // Check if the 'shutdown' signal has been received.  The
    // stage must not release the mutex between this check and the
    // call to wait on the condition variable.
    if (shutdown_) {
      LOG_INFO("received shutdown signal, abandoning timer maintenance\n");
      break;  // !!! EARLY EXIT !!!
    }

    // Sleep until the next service interval.
    first = timer_queue_.begin();
    if (first == timer_queue_.end()) {
      // If no timer events are registered, sleep indefinately.
      // (When new events are registered, the condition variable
      // will be signalled to allow service to resume.)
      LOG_TRACE("sleeping indefinately\n");
      pthread_cond_wait(&timer_condv_, &timer_mutex_);
    } else {
      // If timer events are registered, sleep until the first
      // event should be triggered.
      struct timespec ts;
      ts.tv_sec = first->first.get_time().tv_sec;
      ts.tv_nsec = first->first.get_time().tv_usec * NSEC_PER_USEC;

      LOG_TRACE("sleeping until next deadline: sec=%ld, nsec=%ld\n", ts.tv_sec, ts.tv_nsec);
      pthread_cond_timedwait(&timer_condv_, &timer_mutex_, &ts);
    }
  }

  pthread_mutex_unlock(&timer_mutex_);

  return;
}

bool TimerStage::timer_token_less_than(const TimerToken &tt1, const TimerToken &tt2)
{
  return (tt1 < tt2);
}

}  // namespace common