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

#include <list>

#include "common/defs.h"
#include "common/seda/stage.h"
namespace common {

/**
 *  @file
 *  @author Longda
 *  @date   3/16/07
 */

class Threadpool;

/**
 * A Stage to kill threads in a thread pool
 * The KillThreadStage is scheduled on a thread pool whenever threads
 * need to be killed.  Each event handled by the stage results in the
 * death of the thread.
 */
class KillThreadStage : public Stage {

public:
  /**
   * parse properties, instantiate a summation stage object
   * @pre class members are uninitialized
   * @post initializing the class members
   * @return Stage instantiated object
   */
  static Stage *make_stage(const std::string &tag);

protected:
  /**
   * Constructor
   * @param[in] tag     The label that identifies this stage.
   *
   * @pre  tag is non-null and points to null-terminated string
   * @post event queue is empty
   * @post stage is not connected
   */
  KillThreadStage(const char *tag) : Stage(tag)
  {}

  /**
   * Notify the pool and kill the thread
   * @param[in] event Pointer to event that must be handled.
   *
   * @post  Call never returns.  Thread is killed.  Pool is notified.
   */
  void handle_event(StageEvent *event);

  /**
   * Handle the callback
   * Nothing special for callbacks in this stage.
   */
  void callback_event(StageEvent *event, CallbackContext *context)
  {
    return;
  }

  /**
   * Initialize stage params
   * Ignores next_stage_list_---there are no outputs for this stage.
   *
   * @pre  Stage not connected
   * @return true
   */
  bool initialize()
  {
    return true;
  }

  /**
   * set properties for this object
   * @pre class members are uninitialized
   * @post initializing the class members
   * @return Stage instantiated object
   */
  bool set_properties();

  friend class Threadpool;
};

}  // namespace common
