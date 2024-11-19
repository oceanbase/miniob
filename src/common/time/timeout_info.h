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

#ifndef __COMMON_TIME_TIMEOUT_INFO_H__
#define __COMMON_TIME_TIMEOUT_INFO_H__

#include <time.h>

#include "common/lang/mutex.h"
namespace common {

/**
 * Timeout info class used to judge if a certain deadline_ has reached or not.
 * It's good to use handle-body to automate the reference count
 * increase/decrease. However, explicit attach/detach interfaces
 * are used here to simplify the implementation.
 */

class TimeoutInfo
{
public:
  /**
   * Constructor
   * @param[in] deadline_  deadline_ of this timeout
   */
  TimeoutInfo(time_t deadline_);

  // Increase ref count
  void attach();

  // Decrease ref count
  void detach();

  // Check if it has timed out
  bool has_timed_out();

private:
  // Forbid copy ctor and =() to support ref count

  // Copy constructor.
  TimeoutInfo(const TimeoutInfo &ti);

  // Assignment operator.
  TimeoutInfo &operator=(const TimeoutInfo &ti);

protected:
  // Avoid heap-based \c TimeoutInfo
  // so it can easily associated with \c StageEvent

  // Destructor.
  ~TimeoutInfo();

private:
  time_t deadline_;  // when should this be timed out

  // used to predict timeout if now + reservedTime > deadline_
  // time_t reservedTime;

  bool is_timed_out_;  // timeout flag

  int             ref_cnt_;  // reference count of this object
  pthread_mutex_t mutex_;    // mutex_ to protect ref_cnt_ and flag
};

}  // namespace common
#endif  // __COMMON_TIME_TIMEOUT_INFO_H__
