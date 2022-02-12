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

#include "common/time/timeout_info.h"

#include <sys/time.h>
namespace common {

TimeoutInfo::TimeoutInfo(time_t deadLine) : deadline_(deadLine), is_timed_out_(false), ref_cnt_(0)
{
  MUTEX_INIT(&mutex_, NULL);
}

TimeoutInfo::~TimeoutInfo()
{
  // unlock mutex_ as we locked it before 'delete this'
  MUTEX_UNLOCK(&mutex_);

  MUTEX_DESTROY(&mutex_);
}

void TimeoutInfo::attach()
{
  MUTEX_LOCK(&mutex_);
  ref_cnt_++;
  MUTEX_UNLOCK(&mutex_);
}

void TimeoutInfo::detach()
{
  MUTEX_LOCK(&mutex_);
  if (0 == --ref_cnt_) {
    delete this;
    return;
  }
  MUTEX_UNLOCK(&mutex_);
}

bool TimeoutInfo::has_timed_out()
{
  MUTEX_LOCK(&mutex_);
  bool ret = is_timed_out_;
  if (!is_timed_out_) {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    ret = is_timed_out_ = (tv.tv_sec >= deadline_);
  }
  MUTEX_UNLOCK(&mutex_);

  return ret;
}

}  // namespace common