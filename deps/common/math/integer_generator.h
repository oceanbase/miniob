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
// Created by Wangyunlai on 2023/05/04
//

#pragma once

#include "common/lang/random.h"

namespace common {

class IntegerGenerator
{
public:
  IntegerGenerator(int min, int max) : distrib_(min, max) {}

  IntegerGenerator(const IntegerGenerator &other)       = delete;
  IntegerGenerator(IntegerGenerator &&)                 = delete;
  IntegerGenerator &operator=(const IntegerGenerator &) = delete;

  int next() { return distrib_(rd_); }
  int min() const { return distrib_.min(); }
  int max() const { return distrib_.max(); }

private:
  random_device              rd_;
  uniform_int_distribution<> distrib_;
};

}  // namespace common
