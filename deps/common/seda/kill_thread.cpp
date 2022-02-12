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
#include "common/seda/kill_thread.h"

#include <assert.h>

#include "common/seda/thread_pool.h"
namespace common {

/**
 * Notify the pool and kill the thread
 * @param[in] event Pointer to event that must be handled.
 *
 * @post  Call never returns.  Thread is killed.  Pool is notified.
 */
void KillThreadStage::handle_event(StageEvent *event)
{
  get_pool()->thread_kill();
  event->done();
  this->release_event();
  pthread_exit(0);
}

/**
 * Process properties of the classes
 * @pre class members are uninitialized
 * @post initializing the class members
 * @return the class object
 */
Stage *KillThreadStage::make_stage(const std::string &tag)
{
  return new KillThreadStage(tag.c_str());
}

bool KillThreadStage::set_properties()
{
  // nothing to do
  return true;
}

}  // namespace common