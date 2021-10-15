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
#include "common/seda/event_dispatcher.h"
namespace common {

// Constructor
EventDispatcher::EventDispatcher(const char *tag)
  : Stage(tag), event_store_(), next_stage_(NULL) {
  LOG_TRACE("enter\n");

  pthread_mutexattr_t attr;

  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
  pthread_mutex_init(&event_lock_, &attr);

  LOG_TRACE("exit\n");
}

// Destructor
EventDispatcher::~EventDispatcher() {
  LOG_TRACE("enter\n");
  pthread_mutex_destroy(&event_lock_);
  LOG_TRACE("exit\n");
}

/**
 * Process an event
 * Check if the event can be dispatched. If not, hash it and store
 * it.  If so, send it on to the next stage.
 */
void EventDispatcher::handle_event(StageEvent *event) {
  LOG_TRACE("enter\n");

  std::string hash;
  DispatchContext *ctx = NULL;
  status_t stat;

  pthread_mutex_lock(&event_lock_);
  stat = dispatch_event(event, ctx, hash);
  if (stat == SEND_EVENT) {
    next_stage_->add_event(event);
  } else if (stat == STORE_EVENT) {
    StoredEvent se(event, ctx);

    event_store_[hash].push_back(se);
  } else {
    LOG_ERROR("Dispatch event failure\n");
    // in this case, dispatch_event is assumed to have disposed of event
  }
  pthread_mutex_unlock(&event_lock_);

  LOG_TRACE("exit\n");
}

// Initialize stage params and validate outputs
bool EventDispatcher::initialize() {
  bool ret_val = true;

  if (next_stage_list_.size() != 1) {
    ret_val = false;
  } else {
    next_stage_ = *(next_stage_list_.begin());
  }
  return ret_val;
}

/**
 * Cleanup stage after disconnection
 * Call done() on any events left over in the event_store_.
 */
void EventDispatcher::cleanup() {
  pthread_mutex_lock(&event_lock_);

  // for each hash chain...
  for (EventHash::iterator i = event_store_.begin(); i != event_store_.end(); i++) {

    // for each event on the chain
    for (std::list<StoredEvent>::iterator j = i->second.begin();
         j != i->second.end(); j++) {
      j->first->done();
    }
    i->second.clear();
  }
  event_store_.clear();

  pthread_mutex_unlock(&event_lock_);
}

// Wake up a stored event
bool EventDispatcher::wakeup_event(std::string hashkey) {
  bool sent = false;
  EventHash::iterator i;

  i = event_store_.find(hashkey);
  if (i != event_store_.end()) {

    // find the event and remove it from the current queue
    StoredEvent target_ev = *(i->second.begin());
    i->second.pop_front();
    if (i->second.size() == 0) {
      event_store_.erase(i);
    }

    // try to dispatch the event again
    status_t stat = dispatch_event(target_ev.first, target_ev.second, hashkey);
    if (stat == SEND_EVENT) {
      next_stage_->add_event(target_ev.first);
      sent = true;
    } else if (stat == STORE_EVENT) {
      event_store_[hashkey].push_back(target_ev);
    } else {
      LOG_ERROR("Dispatch event failure\n");
      // in this case, dispatch_event is assumed to have disposed of event
    }
  }

  return sent;
}

} //namespace common