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

#include "common/metrics/console_reporter.h"

#include <iostream>
#include <string>

#include "common/metrics/metric.h"

namespace common {

ConsoleReporter *get_console_reporter()
{
  static ConsoleReporter *instance = new ConsoleReporter();

  return instance;
}

void ConsoleReporter::report(const std::string &tag, Metric *metric)
{
  Snapshot *snapshot = metric->get_snapshot();

  if (snapshot != NULL) {
    printf("%s:%s\n", tag.c_str(), snapshot->to_string().c_str());
  } else {
    printf("There is no snapshot of %s metrics.", tag.c_str());
  }
}

}  // namespace common