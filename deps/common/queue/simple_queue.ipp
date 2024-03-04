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

namespace common {

template <typename T>
int SimpleQueue<T>::push(T &&value)
{
  std::lock_guard<std::mutex> lock(mutex_);
  queue_.push(std::move(value));
  return 0;
}

template <typename T>
int SimpleQueue<T>::pop(T &value)
{
  std::lock_guard<std::mutex> lock(mutex_);
  if (queue_.empty()) {
    return -1;
  }

  value = std::move(queue_.front());
  queue_.pop();
  return 0;
}

template <typename T>
int SimpleQueue<T>::size() const
{
  return queue_.size();
}

} // namespace common