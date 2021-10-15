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



#include "common/metrics/metrics_registry.h"
#include "common/log/log.h"

namespace common {

MetricsRegistry& get_metrics_registry() {
  static MetricsRegistry instance;

  return instance;
}

void MetricsRegistry::register_metric(const std::string &tag, Metric *metric) {
  std::map<std::string, Metric*>::iterator it = metrics.find(tag);
  if (it != metrics.end()) {
    LOG_WARN("%s has been registered!", tag.c_str());
    return;
  }

  //metrics[tag] = metric;
  metrics.insert(std::pair<std::string, Metric *>(tag, metric));
  LOG_INFO("Successfully register metric :%s", tag.c_str());
}

void MetricsRegistry::unregister(const std::string &tag) {
  unsigned int num = metrics.erase(tag);
  if (num == 0) {
    LOG_WARN("There is no %s metric!", tag.c_str());
    return;
  }
  LOG_INFO("Successfully remove metric of %s", tag.c_str());
}

void MetricsRegistry::snapshot() {
  std::map<std::string, Metric*>::iterator it = metrics.begin();
  for (; it != metrics.end(); it++) {
    it->second->snapshot();
  }
}

void MetricsRegistry::report() {
  for (std::list<Reporter *>::iterator reporterIt = reporters.begin();
       reporterIt != reporters.end(); reporterIt++) {
    for (std::map<std::string, Metric*>::iterator it = metrics.begin();
         it != metrics.end(); it++) {

      (*reporterIt)->report(it->first, it->second);
    }
  }
}

} // namespace common
