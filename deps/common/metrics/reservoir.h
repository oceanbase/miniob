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
// Created by Longda on 2021/4/19.
//

#ifndef __COMMON_METRICS_RESERVOIR_H_
#define __COMMON_METRICS_RESERVOIR_H_

#include <stdint.h>

#include "common/math/random_generator.h"
#include "common/metrics/metric.h"
#include "common/metrics/snapshot.h"

namespace common {

class Reservoir : public Metric
{
public:
  Reservoir(RandomGenerator &random);
  virtual ~Reservoir();

public:
  virtual size_t size()      = 0;
  virtual size_t get_count() = 0;

  virtual void update(double one) = 0;

  virtual void reset() = 0;

protected:
  virtual size_t next(size_t range);

private:
  RandomGenerator &random;
};

}  // namespace common

#endif /* __COMMON_METRICS_RESERVOIR_H_ */
