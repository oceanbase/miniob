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

#ifndef __COMMON_METRICS_LOG_REPORTER_H__
#define __COMMON_METRICS_LOG_REPORTER_H__

#include "common/metrics/reporter.h"

namespace common {

class LogReporter : public Reporter {
public:
  void report(const std::string &tag, Metric *metric);
};

LogReporter *get_log_reporter();
}  // namespace common
#endif  //__COMMON_METRICS_LOG_REPORTER_H__
