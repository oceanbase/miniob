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
//
// Created by Wangyunlai
//

#pragma once

#include "common/lang/iterator.h"

namespace common {
/**
 * @brief 找到不小于指定值的元素
 * @note  数组中的元素都不相等
 * @param first 已经排序的序列中第一个元素
 * @param last  已经排序的序列中的最后一个元素的后一个
 * @param val   要查找的元素的值
 * @param comp  比较函数。相等返回0，小于返回负数，大于返回正数
 * @param _found 如果给定，会返回是否存在于当前的序列中
 * @return ForwardIterator 指向lower bound结果的iterator。如果大于最大的值，那么会指向last
 */
template <typename ForwardIterator, typename T, typename Compare>
ForwardIterator lower_bound(
    ForwardIterator first, ForwardIterator last, const T &val, Compare comp, bool *_found = nullptr)
{
  bool            found = false;
  ForwardIterator iter;
  const auto      count      = distance(first, last);
  auto            last_count = count;
  while (last_count > 0) {
    iter      = first;
    auto step = last_count / 2;
    advance(iter, step);
    int result = comp(*iter, val);
    if (0 == result) {
      first = iter;
      found = true;
      break;
    }
    if (result < 0) {
      first = ++iter;
      last_count -= step + 1;
    } else {
      last_count = step;
    }
  }

  if (_found) {
    *_found = found;
  }
  return first;
}

template <typename T>
class Comparator
{
public:
  int operator()(const T &v1, const T &v2) const
  {
    if (v1 < v2) {
      return -1;
    }
    if (v1 > v2) {
      return 1;
    }
    return 0;
  }
};

template <typename ForwardIterator, typename T>
ForwardIterator lower_bound(ForwardIterator first, ForwardIterator last, const T &val, bool *_found = nullptr)
{
  return lower_bound<ForwardIterator, T, Comparator<T>>(first, last, val, Comparator<T>(), _found);
}

// std::iterator is deprecated
// refer to
// https://www.fluentcpp.com/2018/05/08/std-iterator-deprecated/#:~:text=std%3A%3Aiterator%20is%20deprecated%2C%20so%20we%20should%20stop%20using,the%205%20aliases%20inside%20of%20your%20custom%20iterators.
// a sample code:
// https://github.com/google/googletest/commit/25208a60a27c2e634f46327595b281cb67355700
template <typename T, typename Distance = ptrdiff_t>
class BinaryIterator
{
public:
  using iterator_category = random_access_iterator_tag;
  using value_type        = T;
  using difference_type   = Distance;
  using pointer           = value_type *;
  using reference         = value_type &;

public:
  BinaryIterator() = default;
  BinaryIterator(size_t item_num, T *data) : item_num_(item_num), data_(data) {}

  BinaryIterator &operator+=(int n)
  {
    data_ += (item_num_ * n);
    return *this;
  }
  BinaryIterator &operator-=(int n) { return this->operator+(-n); }
  BinaryIterator &operator++() { return this->operator+=(1); }
  BinaryIterator  operator++(int)
  {
    BinaryIterator tmp(*this);
    this->         operator++();
    return tmp;
  }
  BinaryIterator &operator--() { return this->operator+=(-1); }
  BinaryIterator  operator--(int)
  {
    BinaryIterator tmp(*this);
    *this += -1;
    return tmp;
  }

  bool operator==(const BinaryIterator &other) const { return data_ == other.data_; }
  bool operator!=(const BinaryIterator &other) const { return !(this->operator==(other)); }

  T *operator*() { return data_; }
  T *operator->() { return data_; }

  friend Distance operator-(const BinaryIterator &left, const BinaryIterator &right)
  {
    return (left.data_ - right.data_) / left.item_num_;
  }

private:
  size_t item_num_ = 0;
  T     *data_     = nullptr;
};
}  // namespace common
