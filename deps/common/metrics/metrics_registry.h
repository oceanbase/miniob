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
// Created by Longda on 2021/4/20.
//

#ifndef __COMMON_METRICS_METRICS_REGISTRY_H__
#define __COMMON_METRICS_METRICS_REGISTRY_H__

#include <string>
#include <map>
#include <list>

#include "common/metrics/metric.h"
#include "common/metrics/reporter.h"

namespace common {

class MetricsRegistry {
public:
  MetricsRegistry() {};
  virtual ~MetricsRegistry(){};

  void register_metric(const std::string &tag, Metric *metric);
  void unregister(const std::string &tag);

  void snapshot();

  void report();

  void add_reporter(Reporter *reporter) {
    reporters.push_back(reporter);
  }


protected:
  std::map<std::string, Metric *> metrics;
  std::list<Reporter *> reporters;


};

MetricsRegistry& get_metrics_registry();
}//namespace common
#endif //__COMMON_METRICS_METRICS_REGISTRY_H__
