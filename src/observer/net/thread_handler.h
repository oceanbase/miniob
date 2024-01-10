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
// Created by Wangyunlai on 2024/01/10.
//

#pragma once

#include <functional>
#include "common/rc.h"

class Communicator;

class ThreadHandler
{
public:
  ThreadHandler() = default;
  virtual ~ThreadHandler() = default;

  virtual RC new_connection(Communicator *communicator) = 0;
  virtual RC close_connection(Communicator *communicator) = 0;

public:
  static ThreadHandler * create(const char *name);
};