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
// Created by Longda on 2021/4/19.
//

#ifndef __COMMON_METRICS_METRICS_H__
#define __COMMON_METRICS_METRICS_H__

#include "common/lang/string.h"
#include "common/metrics/metric.h"
#include "common/metrics/snapshot.h"
#include "common/metrics/timer_snapshot.h"
#include "common/metrics/uniform_reservoir.h"
#include <sys/time.h>

namespace common {

class Gauge : public Metric {
public:
  // user implement snapshot function
  void set_snapshot(Snapshot *value) { snapshot_value_ = value; }
};

class Counter : public Metric {
  void set_snapshot(SnapshotBasic<long> *value) { snapshot_value_ = value; }
};

class Meter : public Metric {
public:
  Meter();
  virtual  ~Meter();

  void inc(long increase);
  void inc();

  void snapshot();

protected:
  std::atomic<long> value_;
  long snapshot_tick_;
};

// SimpleTimer just get tps and meanvalue
// time unit is ms
class SimpleTimer : public Meter {
public:
  virtual ~SimpleTimer();

  void inc(long increase);

  void update(long one);

  void snapshot();

protected:
  std::atomic<long> times_;
};

// Histogram metric is complicated, in normal case ,
//  please skip us histogram or Timer as more as possible
//  try use SimpleTimer to replace them.
//  if use histogram , please use sampling method.
class Histogram : public UniformReservoir {
public:
  Histogram(RandomGenerator &random);
  Histogram(RandomGenerator &random, size_t size);
  virtual ~Histogram();

  void snapshot();

};

// timeunit is ms
// Timer = Histogram + Meter
class Timer : public UniformReservoir {
public:
  Timer(RandomGenerator &random);
  Timer(RandomGenerator &random, size_t size);
  virtual ~Timer();

  void snapshot();
  void update(double ms);

protected:
  std::atomic<long> value_;
  long snapshot_tick_;
};
// update ms
class TimerStat {
public:
  TimerStat(SimpleTimer &st_);

  ~TimerStat();
  void start();
  void end();

public:
  SimpleTimer &st_;
  long start_tick_;
  long end_tick_;
};

} // namespace common
#endif //__COMMON_METRICS_METRICS_H__
