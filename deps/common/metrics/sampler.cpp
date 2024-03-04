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

#include "common/metrics/sampler.h"
#include "common/log/log.h"

#define RANGE_SIZE 100

namespace common {

Sampler *&get_sampler()
{
  static Sampler *g_sampler = new Sampler();

  return g_sampler;
}

Sampler::Sampler() : random_() {}

Sampler::~Sampler() {}

bool Sampler::sampling()
{
  int v = random_.next(RANGE_SIZE);
  if (v <= ratio_num_) {
    return true;
  } else {
    return false;
  }
}

double Sampler::get_ratio() { return ratio_; }

void Sampler::set_ratio(double ratio)
{
  if (0 <= ratio && ratio <= 1) {
    this->ratio_ = ratio;
    ratio_num_   = ratio * RANGE_SIZE;
  } else {
    LOG_WARN("Invalid ratio :%lf", ratio);
  }
}

}  // namespace common
