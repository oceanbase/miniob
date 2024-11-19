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
// Created by wangyunlai.wyl on 2024/02/19.
//

#pragma once

#include <stdint.h>

#include "common/lang/vector.h"
#include "common/lang/span.h"

namespace common {

/**
 * @brief 序列化工具
 * @details 这个设计可以拆分开更精确，Buffer + Serializer
 */
class Serializer final
{
public:
  using BufferType = vector<char>;

public:
  Serializer()  = default;
  ~Serializer() = default;

  Serializer(const Serializer &)            = delete;
  Serializer &operator=(const Serializer &) = delete;

  /// @brief 写入指定长度的数据
  int write(span<const char> data);
  /// @brief 写入指定长度的数据
  int write(const char *data, int size) { return write(span<const char>(data, size)); }
  /// @brief 当前写入了多少数据
  int64_t size() const { return buffer_.size(); }

  BufferType       &data() { return buffer_; }
  const BufferType &data() const { return buffer_; }

  /// @brief 写入一个int32整数
  int write_int32(int32_t value);
  /// @brief 写入一个int64整数
  int write_int64(int64_t value);

private:
  BufferType buffer_;
};

/**
 * @brief 反序列化工具
 */
class Deserializer final
{
public:
  explicit Deserializer(span<const char> buffer) : buffer_(buffer) {}
  Deserializer(const char *buffer, int size) : buffer_(buffer, size) {}
  ~Deserializer() = default;

  Deserializer(const Deserializer &)            = delete;
  Deserializer &operator=(const Deserializer &) = delete;

  /// @brief 读取指定大小的数据
  int read(span<char> data);
  /// @brief 读取指定长度的数据
  int read(char *data, int size) { return read(span<char>(data, size)); }

  /// @brief buffer的大小
  int64_t size() const { return buffer_.size(); }

  /// @brief 还剩余多少数据
  int64_t remain() const { return buffer_.size() - position_; }

  /// @brief 读取一个int32数据
  int read_int32(int32_t &value);
  /// @brief 读取一个int64数据
  int read_int64(int64_t &value);

private:
  span<const char> buffer_;        ///< 存放数据的buffer
  int64_t          position_ = 0;  ///< 当前读取到的位置
};

}  // namespace common
