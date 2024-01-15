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

#include <queue>
#include <mutex>
#include "common/queue/queue.h"

namespace common {

/**
 * @brief 一个十分简单的线程安全的任务队列
 * @details 所有的接口都加了一个锁来保证线程安全。
 * 如果想了解更高效的队列实现，请参考 [Oceanbase](https://github.com/oceanbase/oceanbase) 中
 * deps/oblib/src/lib/queue/ 的一些队列的实现
 * @tparam T 任务数据类型。
 * @ingroup Queue
 */
template <typename T>
class SimpleQueue : public Queue<T>
{
public:
  using value_type = T;

public:
  SimpleQueue() : Queue<T>() {}
  virtual ~SimpleQueue() {}

  //! @copydoc Queue::emplace
  int push(value_type &&value) override;
  //! @copydoc Queue::pop
  int pop(value_type &value) override;
  //! @copydoc Queue::size
  int size() const override;

private:
  std::mutex             mutex_;
  std::queue<value_type> queue_;
};

}  // namespace common

#include "common/queue/simple_queue.ipp"