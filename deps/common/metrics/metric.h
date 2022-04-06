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

#ifndef __COMMON_METRICS_METRIC_H__
#define __COMMON_METRICS_METRIC_H__

#include "common/metrics/snapshot.h"

namespace common {

class Metric {
public:
  virtual void snapshot() = 0;

  virtual Snapshot *get_snapshot()
  {
    return snapshot_value_;
  }

protected:
  Snapshot *snapshot_value_;
};

}  // namespace common
#endif  //__COMMON_METRICS_METRIC_H__
