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
#include "common/seda/stage.h"

#include <assert.h>
#include <string.h>

#include "common/defs.h"
#include "common/lang/mutex.h"
#include "common/lang/string.h"
#include "common/log/log.h"
#include "common/seda/init.h"
#include "common/seda/thread_pool.h"
namespace common {


/**
 * Constructor
 * @param[in] tag     The label that identifies this stage.
 *
 * @pre  tag is non-null and points to null-terminated string
 * @post event queue is empty
 * @post stage is not connected
 */
Stage::Stage(const char *tag)
    : next_stage_list_(), event_list_(), connected_(false), event_ref_(0) {
  LOG_TRACE("%s", "enter");
  assert(tag != NULL);

  MUTEX_INIT(&list_mutex_, NULL);
  COND_INIT(&disconnect_cond_, NULL);
  stage_name_ = new char[strlen(tag) + 1];
  snprintf(stage_name_, strlen(tag) + 1, "%s", tag);
  LOG_TRACE("%s", "exit");
}

/**
 * Destructor
 * @pre  stage is not connected
 * @post pending events are deleted and stage is destroyed
 */
Stage::~Stage() {
  LOG_TRACE("%s", "enter");
  assert(!connected_);
  MUTEX_LOCK(&list_mutex_);
  while (event_list_.size() > 0) {
    delete *(event_list_.begin());
    event_list_.pop_front();
  }
  MUTEX_UNLOCK(&list_mutex_);
  next_stage_list_.clear();

  MUTEX_DESTROY(&list_mutex_);
  COND_DESTROY(&disconnect_cond_);
  delete[] stage_name_;
  LOG_TRACE("%s", "exit");
}

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
 * @return true if the connection succeeded, else false
 */
bool Stage::connect() {
  LOG_TRACE("%s%s", "enter", stage_name_);
  assert(!connected_);
  assert(th_pool_ != NULL);

  bool success = false;
  unsigned int backlog = 0;

  success = initialize();
  if (success) {
    MUTEX_LOCK(&list_mutex_);
    backlog = event_list_.size();
    event_ref_ = backlog;
    connected_ = true;
    MUTEX_UNLOCK(&list_mutex_);
  }

  // if connection succeeded, schedule all the events in the queue
  if (connected_) {
    while (backlog > 0) {
      th_pool_->schedule(this);
      backlog--;
    }
  }

  LOG_TRACE("%s%s%d", "exit", stage_name_, connected_);
  return success;
}

/**
 * Disconnect this stage from the pipeline and threadpool
 * Block stage from being scheduled.  Wait for currently processing
 * and scheduled events to complete, then disconnect from the threadpool.
 * Disconnect the output of this stage from the inputs of the stages in the
 * next_stage_list_.
 *
 * @pre    stage is connected
 * @post   next_stage_list_ empty
 * @post   th_pool_ NULL
 * @post   stage is not connected
 */
void Stage::disconnect() {
  assert(connected_ == true);

  LOG_TRACE("%s%s", "enter", stage_name_);
  MUTEX_LOCK(&list_mutex_);
  disconnect_prepare();
  connected_ = false;
  while (event_ref_ > 0) {
    COND_WAIT(&disconnect_cond_, &list_mutex_);
  }
  th_pool_ = NULL;
  next_stage_list_.clear();
  cleanup();
  MUTEX_UNLOCK(&list_mutex_);
  LOG_TRACE("%s%s", "exit", stage_name_);
}

/**
 * Add an event to the queue.
 * @param[in] event Event to add to queue.
 *
 * @pre  event non-null
 * @post event added to the end of event queue
 * @post event must not be de-referenced by caller after return
 */
void Stage::add_event(StageEvent *event) {
  assert(event != NULL);

  MUTEX_LOCK(&list_mutex_);

  // add event to back of queue
  event_list_.push_back(event);

  if (connected_) {
    assert(th_pool_ != NULL);

    event_ref_++;
    MUTEX_UNLOCK(&list_mutex_);
    th_pool_->schedule(this);
  } else {
    MUTEX_UNLOCK(&list_mutex_);
  }
}

/**
 * Query length of queue
 * @return length of event queue.
 */
unsigned long Stage::qlen() const {
  unsigned long res;

  MUTEX_LOCK(&list_mutex_);
  res = event_list_.size();
  MUTEX_UNLOCK(&list_mutex_);
  return res;
}

/**
 * Query whether the queue is empty
 * @return \c true if the queue is empty; \c false otherwise
 */
bool Stage::qempty() const {
  bool empty = false;

  MUTEX_LOCK(&list_mutex_);
  empty = event_list_.empty();
  MUTEX_UNLOCK(&list_mutex_);
  return empty;
}

/**
 * Remove an event from the queue.  Called only by service thread.
 *
 * @pre queue not empty.
 * @return first event on queue.
 * @post  first event on queue is removed from queue.
 */
StageEvent *Stage::remove_event() {
  MUTEX_LOCK(&list_mutex_);

  assert(!event_list_.empty());

  StageEvent *se = *(event_list_.begin());
  event_list_.pop_front();
  MUTEX_UNLOCK(&list_mutex_);

  return se;
}

/**
 * Release event reference on stage.  Called only by service thread.
 *
 * @post event ref count on stage is decremented
 */
void Stage::release_event() {
  MUTEX_LOCK(&list_mutex_);
  event_ref_--;
  if (!connected_ && event_ref_ == 0) {
    COND_SIGNAL(&disconnect_cond_);
  }
  MUTEX_UNLOCK(&list_mutex_);
}

} //namespace common