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

class SerializeBuffer final
{
public:
  using BufferType = std::vector<char>;

public:
  SerializeBuffer() = default;
  ~SerializeBuffer() = default;

  SerializeBuffer(const SerializeBuffer &) = delete;
  SerializeBuffer &operator=(const SerializeBuffer &) = delete;

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

} // namespace common