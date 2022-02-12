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

#ifndef __COMMON_METRICS_SAMPLER_H__
#define __COMMON_METRICS_SAMPLER_H__

#include "common/math/random_generator.h"

namespace common {

/**
 * The most simple sample function
 */
class Sampler {
public:
  Sampler();
  virtual ~Sampler();

  bool sampling();

  void set_ratio(double ratio);
  double get_ratio();

private:
  double ratio_ = 1.0;
  int ratio_num_ = 1;
  RandomGenerator random_;
};

Sampler *&get_sampler();
}  // namespace common
#endif  //__COMMON_METRICS_SAMPLER_H__
