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

#include <vector>
#include <span>

namespace common {

/**
 * @brief 序列化工具
 * @details 这个设计可以拆分开更精确，Buffer + Serializer
 */
class Serializer final
{
public:
  using BufferType = std::vector<char>;

public:
  Serializer() = default;
  ~Serializer() = default;

  Serializer(const Serializer &) = delete;
  Serializer &operator=(const Serializer &) = delete;

  int write(std::span<const char> data);
  int write(const char *data, int size) { return write(std::span<const char>(data, size)); }
  int64_t size() const { return buffer_.size(); }

  BufferType &data()  { return buffer_; }
  const BufferType &data() const { return buffer_; }

  int write_int32(int32_t value);
  int write_int64(int64_t value);

private:
  BufferType buffer_;
};

class Deserializer final
{
public:
  explicit Deserializer(std::span<const char> buffer) : buffer_(buffer) {}
  Deserializer(const char *buffer, int size) : buffer_(buffer, size) {}
  ~Deserializer() = default;

  Deserializer(const Deserializer &) = delete;
  Deserializer &operator=(const Deserializer &) = delete;

  int read(std::span<char> data);
  int read(char *data, int size) { return read(std::span<char>(data, size)); }
  int64_t size() const { return buffer_.size(); }
  int64_t remain() const { return buffer_.size() - position_; }

  int read_int32(int32_t &value);
  int read_int64(int64_t &value);

private:
  std::span<const char> buffer_;
  int64_t position_ = 0;
};

} // namespace common