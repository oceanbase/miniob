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
// Created by Wangyunlai on 2023/01/11.
//

#pragma once

#include "common/lang/functional.h"

namespace common {

/**
 * @brief 可执行对象接口
 * @ingroup ThreadPool
 */
class Runnable
{
public:
  Runnable()          = default;
  virtual ~Runnable() = default;

  virtual void run() = 0;
};

/**
 * @brief 可执行对象适配器，方便使用lambda表达式
 * @ingroup ThreadPool
 */
class RunnableAdaptor : public Runnable
{
public:
  RunnableAdaptor(function<void()> callable) : callable_(callable) {}

  void run() override { callable_(); }

private:
  function<void()> callable_;
};

}  // namespace common
