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
#pragma once

#include <pthread.h>

#include <atomic>
#include <vector>

#include "common/metrics/reservoir.h"

namespace common {

/**
 * A random sampling reservoir of a stream of {@code long}s. Uses Vitter's
 * Algorithm R to produce a statistically representative sample.
 *
 * @see <a href="http://www.cs.umd.edu/~samir/498/vitter.pdf">Random Sampling
 * with a Reservoir</a>
 */

class UniformReservoir : public Reservoir
{
public:
  UniformReservoir(RandomGenerator &random);
  UniformReservoir(RandomGenerator &random, size_t size);
  virtual ~UniformReservoir();

public:
  size_t size();       // data buffer size
  size_t get_count();  // how many items have been insert?

  void update(double one);
  void snapshot();

  void reset();

protected:
  void init(size_t size);

protected:
  pthread_mutex_t     mutex;
  size_t              counter;  // counter is likely to be bigger than data.size()
  std::vector<double> data;
  RandomGenerator     random;
};

}  // namespace common
