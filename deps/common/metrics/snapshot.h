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

#ifndef __COMMON_METRICS_SNAPSHOT_H__
#define __COMMON_METRICS_SNAPSHOT_H__

#include <string>
#include "common/lang/string.h"

namespace common {


class Snapshot {
public:
  virtual ~Snapshot() {};
  virtual std::string to_string() = 0;
};

template <class T>
class SnapshotBasic : public Snapshot {
public:
  SnapshotBasic() : value(){

  };

  virtual ~SnapshotBasic() {}

  void setValue(T &input) { value = input; }

  std::string to_string() {
    std::string ret;
    val_to_str(value, ret);
    return ret;
  }

private:
  T value;
};

class SimplerTimerSnapshot: public  Snapshot{
public:
   SimplerTimerSnapshot() {

  }

  virtual ~SimplerTimerSnapshot() {}

  void setValue(double mean, double tps) {
    this->mean = mean;
    this->tps = tps;
  }

  std::string to_string() {
    std::stringstream oss;
    oss << "mean:" << mean << ",tps:"<<tps;

    return oss.str();
  }
private:
  double mean = 1.0;
  double tps = 1.0;
};
} //namespace common
#endif //__COMMON_METRICS_SNAPSHOT_H__
