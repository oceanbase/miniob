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
// Created by Longda on 2021/4/20.
//

#include "common/metrics/timer_snapshot.h"
#include <sstream>

namespace common {

TimerSnapshot::TimerSnapshot()
{}

TimerSnapshot::~TimerSnapshot()
{}

double TimerSnapshot::get_tps()
{
  return tps;
}

void TimerSnapshot::set_tps(double tps)
{
  this->tps = tps;
}

std::string TimerSnapshot::to_string()
{
  std::stringstream oss;

  oss << HistogramSnapShot::to_string() << ",tps:" << tps;

  return oss.str();
}
}  // namespace common