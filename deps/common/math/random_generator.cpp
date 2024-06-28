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

#include "common/math/random_generator.h"
#include "common/lang/chrono.h"

namespace common {

RandomGenerator::RandomGenerator() : randomData(chrono::system_clock::now().time_since_epoch().count()) {}

RandomGenerator::~RandomGenerator() {}

unsigned int RandomGenerator::next() { return randomData(); }

unsigned int RandomGenerator::next(unsigned int range)
{
  if (range > 0) {
    return next() % range;
  } else {
    return 0;
  }
}

}  // namespace common