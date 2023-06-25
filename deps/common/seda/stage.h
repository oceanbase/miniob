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

#ifndef __COMMON_SEDA_STAGE_H__
#define __COMMON_SEDA_STAGE_H__

// Include Files
#include <deque>
#include <list>

// project headers
#include "common/defs.h"
#include "common/log/log.h"

// seda headers
#include "common/seda/stage_event.h"
namespace common {

class Threadpool;
class CallbackContext;

/**
 * A Stage in a staged event-driven architecture
 * The Stage class consists of a queue of events and a link to a
 * Threadpool.  The threads in the pool handle the events on the queue.
 * The key method of the Stage class is handle_event(), which is abstract.
 * Each subclass of Stage will fill in the handle_event() method with
 * appropriate code for that stage. The expected behavior of handle_event()
 * is to perform stage-specific processing for the event argument, and
 * complete by passing the event on to the queue of the next appropriate
 * stage. Occasionally, handle_event may cause a new event to be created,
 * or the current event to be freed.  The generic Stage class handles
 * maintenance of the event queue, and scheduling the stage on the runqueue
 * of the Threadpool as long as events are in the queue.
 * Subclasses of Stage may also add data members that represent
 * stage-specific state and are accessed from the handle_event() method.
 * <p>
 * A stage can register a callback on an event that it passes to another
 * stage later in the pipeline.  When the callback is invoked, it calls
 * the callback_event() virtual function from the stage that set the
 * callback.
 * <p>
 * An event is an argument to the handle_event() method.
 * The Event class is generic and is intended to be specialized by
 * subclasses in order to represent real events.  handle_event() is
 * expected to use C++ introspection (dynamic cast) to determine the
 * specific sub-class of event that it is dealing with on each invocation.
 * <p>
 * After a Stage is constructed, nothing happens because the stage is not
 * yet initialized.  At this point the queue is empty and the stage is not
 * yet connected to any Threadpool or Stage pipeline.  In general, init
 * and configuration of stages is handled by the SedaConfig class which
 * picks up configuration information from a central config file.  Processing
 * for the stage does not start until the SedaConfig class calls connect().
 * connect() hooks the stage up to a Threadpool and connects the output of
 * the stage to the input of some other stages. A single Threadpool can be
 * dedicated to a single Stage, or multiple Stages can share the same
 * Threadpool.  disconnect() performs the opposite task, breaking the
 * connection between a Stage and its Threadpool and pipeline successors.
 * disconnect is currently called by the SedaConfig object on each Stage
 * in the pipeline when the SedaConfig object is destroyed.
 *
 * There are three virtual functions that allow a stage class to customize
 * the initialization/finalization process.  set_properties() is called
 * by the SedaConfig object immediately after the stage is created, and
 * is used to pass stage-specific configuration options via a Properties
 * class.  initialize() is called from connect().  The purpose of
 * initialize() is to allow the stage class to verify that the next_stage_list_
 * contains valid stages (number, type, etc), and to initialize any stage
 * state (such as aliases for members of the next stage list) prior to
 * stage connection.  cleanup() is called from disconnect to allow the
 * stage class to release resources before entering the disconnected state.
 * initialize() and cleanup() should be coded so that a stage can be
 * repeatedly disconnected then re-connected and continue to function
 * properly.
 */
class Stage {

  // public interface operations

public:
  /**
   * Destructor
   * @pre  stage is not connected
   * @post pending events are deleted and stage is destroyed
   */
  virtual ~Stage();

  /**
   * parse properties, instantiate a summation stage object
   * @pre class members are uninitialized
   * @post initializing the class members
   * @return Stage instantiated object
   */
  static Stage *make_stage(const std::string &tag);

  /**
   * Return the Threadpool object
   * @return reference to the Threadpool for this Stage
   */
  Threadpool *get_pool()
  {
    return th_pool_;
  }

  /**
   * Push stage to the list of the next stages
   * @param[in] stage pointer
   *
   * @pre  stage not connected
   * @post added a new stage
   */
  void push_stage(Stage *);

  /**
   * Get name of the stage
   * @return return the name of this stage which is the class name
   */
  const char *get_name();

  /**
   * Set threadpool
   * @param[in] threadpool pointer
   *
   * @pre  stage not connected
   * @post seed threadpool for this stage
   */
  void set_pool(Threadpool *);

  /**
   * Connect this stage to pipeline and threadpool
   * Connect the output of this stage to the inputs of the stages in
   * the provided stage list.  Each subclass will validate the provided
   * stage list to be sure it is appropriate.  If the validation succeeds,
   * connect to the Threadpool and start processing.
   *
   * @param[in,out] stageList Stages that come next in the pipeline.
   * @param[in]     pool      Threadpool which will handle events
   *
   * @pre  stage is not connected
   * @post next_stage_list_ == original stageList
   * @post stageList is empty
   * @post th_pool_ == pool
   * @return TRUE if the connection succeeded, else FALSE
   */
  bool connect();

  /**
   * Disconnect this stage from the pipeline and threadpool
   * Block stage from being scheduled.  Wait for currently processing
   * and queued events to complete, then disconnect from the threadpool.
   * Disconnect the output of this stage from the inputs of the stages in
   * the next_stage_list_.
   *
   * @pre    stage is connected
   * @post   next_stage_list_ empty
   * @post   th_pool_ NULL
   * @post   stage is not connected
   */
  void disconnect();

  /**
   * Add an event to the queue.
   * This will trigger thread switch, you can use handle_event without thread switch
   * @param[in] event Event to add to queue.
   *
   * @pre  event non-null
   * @post event added to the end of event queue
   * @post event must not be de-referenced by caller after return
   * @post event ref count on stage is incremented
   */
  void add_event(StageEvent *event);

  /**
   * Query length of queue
   * @return length of event queue.
   */
  unsigned long qlen() const;

  /**
   * Query whether the queue is empty
   * @return \c true if the queue is empty; \c false otherwise
   */
  bool qempty() const;

  /**
   * Query whether stage is connected
   * @return true if stage is connected
   */
  bool is_connected() const
  {
    return connected_;
  }

  /**
   * Perform Stage-specific processing for an event
   * Processing one event without switch thread.
   * Handle the event according to requirements of specific stage.  Pure
   * virtual member function.
   *
   * @param[in] event Pointer to event that must be handled.
   *
   * @post  event must not be de-referenced by caller after return
   */
  virtual void handle_event(StageEvent *event) = 0;

  /**
   * Perform Stage-specific callback processing for an event
   * Implement callback processing according to the requirements of
   * specific stage.  Pure virtual member function.
   *
   * @param[in] event Pointer to event that initiated the callback.
   */
  virtual void callback_event(StageEvent *event, CallbackContext *context)
  {}

  /**
   * Perform Stage-specific callback processing for a timed out event
   * A stage only need to implement this interface if the down-stream
   * stages support event timeout detection.
   */
  virtual void timeout_event(StageEvent *event, CallbackContext *context)
  {
    LOG_INFO("get a timed out evnet in %s timeout_event\n", stage_name_);
    this->callback_event(event, context);
  }

protected:
  /**
   * Constructor
   * @param[in] tag     The label that identifies this stage.
   *
   * @pre  tag is non-null and points to null-terminated string
   * @post event queue is empty
   * @post stage is not connected
   */
  Stage(const char *tag);

  /**
   * Remove an event from the queue. Called only by service thread.
   *
   * @pre queue not empty
   * @return first event on queue.
   * @post  first event on queue is removed from queue.
   */
  StageEvent *remove_event();

  /**
   * Release event reference on stage.  Called only by service thread.
   *
   * @post event ref count on stage is decremented
   */
  void release_event();

  /**
   * Initialize stage params and validate outputs
   * Validate the next_stage_list_ according to the requirements of the
   * specific stage.  Initialize stage specific params.  Pure virtual
   * member function.
   *
   * @pre  Stage not connected
   * @return TRUE if and only if outputs are valid and init succeeded.
   */
  virtual bool initialize()
  {
    return true;
  }

  /**
   * set properties for this object
   * @pre class members are uninitialized
   * @post initializing the class members
   * @return Stage instantiated object
   */
  virtual bool set_properties()
  {
    return true;
  }

  /**
   *  Prepare to disconnect the stage.
   *  This function is called to allow a stage to perform
   *  stage-specific actions in preparation for disconnecting it
   *  from the pipeline.  Most stages will not need to implement
   *  this function.
   */
  virtual void disconnect_prepare()
  {
    return;
  }

  /**
   * Cleanup stage after disconnection
   * After disconnection is completed, cleanup any resources held by the
   * stage and prepare for destruction or re-initialization.
   */
  virtual void cleanup()
  {
    return;
  }

  // pipeline state
  std::list<Stage *> next_stage_list_;  // next stage(s) in the pipeline

  // implementation state
  char *stage_name_;  // name of stage

  friend class Threadpool;

private:
  std::deque<StageEvent *> event_list_;  // event queue
  mutable pthread_mutex_t list_mutex_;   // protects the event queue
  pthread_cond_t disconnect_cond_;       // wait here for disconnect
  bool connected_;                       // is stage connected to pool?
  unsigned long event_ref_;              // # of outstanding events
  Threadpool *th_pool_ = nullptr;        // Threadpool for this stage
};

inline void Stage::set_pool(Threadpool *th)
{
  ASSERT((th != NULL), "threadpool not available for stage %s", this->get_name());
  ASSERT(!connected_, "attempt to set threadpool while connected: %s", this->get_name());
  th_pool_ = th;
}

inline void Stage::push_stage(Stage *st)
{
  ASSERT((st != NULL), "next stage not available for stage %s", this->get_name());
  ASSERT(!connected_, "attempt to set push stage while connected: %s", this->get_name());
  next_stage_list_.push_back(st);
}

inline const char *Stage::get_name()
{
  return stage_name_;
}

}  // namespace common
#endif  // __COMMON_SEDA_STAGE_H__
