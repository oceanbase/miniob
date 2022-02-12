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

#ifndef __COMMON_SEDA_EVENT_DISPATCHER_H__
#define __COMMON_SEDA_EVENT_DISPATCHER_H__

// Include Files
#include <list>
#include <map>

// SEDA headers
#include "common/seda/stage.h"
#include "common/seda/stage_event.h"
namespace common {

/**
 *  @file   Event Dispatcher
 *  @author Longda
 *  @date   8/20/07
 */

class DispatchContext;

/**
 * A stage which stores and re-orders events
 * The EventDispatcher stage is designed to assert control over the order
 * of events that move through the Seda pipeline.  The EventDispatcher stage
 * has a hash table that stores events for later processing.  As each event
 * arrives at the Dispatcher, a test is applied to determine whether it
 * should be allowed to proceed.  This test is implemented in subclasses
 * and uses state from the event and state held in the dispatcher itself.
 * If the event is meant to be delayed, it is hashed and stored.  The
 * dispatcher also provides an interface that "wakes up" a stored, event
 * and re-applies the dispatch test.  This wake-up interface can be called
 * from a background thread, or from a callback associated with an event
 * that has already been dispatched.
 *
 * The EventDispatcher class is designed to be specialized by adding a
 * specific implementation of the dispatch test for events, and a method
 * or process for waking up stored events at the appropriate time.
 */

class EventDispatcher : public Stage {

  // public interface operations

public:
  typedef enum { SEND_EVENT = 0, STORE_EVENT, FAIL_EVENT } status_t;

  /**
   * Destructor
   * @pre  stage is not connected
   * @post pending events are deleted and stage is destroyed
   */
  virtual ~EventDispatcher();

  /**
   * Process an event
   * Check if the event can be dispatched. If not, hash it and store
   * it.  If so, send it on to the next stage
   *
   * @param[in] event Pointer to event that must be handled.
   * @post  event must not be de-referenced by caller after return
   */
  void handle_event(StageEvent *event);

  // Note, EventDispatcher is an abstract class and needs no make_stage()

protected:
  /**
   * Constructor
   * @param[in] tag     The label that identifies this stage.
   *
   * @pre  tag is non-null and points to null-terminated string
   * @post event queue is empty
   * @post stage is not connected
   */
  EventDispatcher(const char *tag);

  /**
   * Initialize stage params and validate outputs
   * @pre  Stage not connected
   * @return TRUE if and only if outputs are valid and init succeeded.
   */
  bool initialize();

  // set properties for this object
  bool set_properties()
  {
    return true;
  }

  /**
   * Cleanup stage after disconnection
   * After disconnection is completed, cleanup any resources held by the
   * stage and prepare for destruction or re-initialization.
   */
  virtual void cleanup();

  /**
   * Dispatch test
   * @param[in] ev  Pointer to event that should be tested
   * @param[in/out]  Pointer to context object
   * @param[out] hash  Hash value for event
   *
   * @pre event_lock_ is locked
   * @post hash is calculated if return val is false
   * @return SEND_EVENT if ok to send the event down the pipeline;
   *                    ctx is NULL
   *         STORE_EVENT if event should be stored; ctx will be stored
   *         FAIL_EVENT if failure, and event has been completed;
   *                    ctx is NULL
   */
  virtual status_t dispatch_event(StageEvent *ev, DispatchContext *&ctx, std::string &hash) = 0;

  /**
   * Wake up a stored event
   * @pre event_lock_ is locked
   * @return true if an event was found on hash-chain associated with
   *              hashkey and sent to next stage
   *         false no event was found on hash-chain
   */
  bool wakeup_event(std::string hashkey);

  // implementation state

  typedef std::pair<StageEvent *, DispatchContext *> StoredEvent;
  typedef std::map<std::string, std::list<StoredEvent>> EventHash;

  EventHash event_store_;       // events stored here while waiting
  pthread_mutex_t event_lock_;  // protects access to event_store_
  Stage *next_stage_;           // target for dispatched events

protected:
};

/**
 * Class to store context info with the stored event.  Subclasses should
 * derive from this base class.
 */
class DispatchContext {
public:
  virtual ~DispatchContext()
  {}
};

}  // namespace common
#endif  // __COMMON_SEDA_EVENT_DISPATCHER_H__
