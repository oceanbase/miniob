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
#ifndef __COMMON_MATH_RANDOM_GENERATOR_H_
#define __COMMON_MATH_RANDOM_GENERATOR_H_

#include <stdlib.h>
#include <random>
namespace common {

#define DEFAULT_RANDOM_BUFF_SIZE 512

class RandomGenerator {

public:
  RandomGenerator();
  virtual ~RandomGenerator();

public:
  unsigned int next();
  unsigned int next(unsigned int range);

private:
  // The GUN Extended TLS Version
  std::mt19937 randomData;
};

}  // namespace common

#endif /* __COMMON_MATH_RANDOM_GENERATOR_H_ */
