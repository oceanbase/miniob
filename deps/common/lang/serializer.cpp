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
// Created by wangyunlai.wyl on 2024/02/20.
//

#include <string.h>

#include "common/lang/serializer.h"

namespace common {
int Serializer::write(span<const char> data)
{
  buffer_.insert(buffer_.end(), data.begin(), data.end());
  return 0;
}

int Serializer::write_int32(int32_t value)
{
  char *p = reinterpret_cast<char *>(&value);
  return write(span(p, sizeof(value)));
}

int Serializer::write_int64(int64_t value)
{
  char *p = reinterpret_cast<char *>(&value);
  return write(span(p, sizeof(value)));
}

int Deserializer::read(span<char> data)
{
  if (static_cast<int64_t>(data.size()) > remain()) {
    return -1;
  }

  memcpy(data.data(), buffer_.data() + position_, data.size());
  position_ += data.size();
  return 0;
}

int Deserializer::read_int32(int32_t &value)
{
  span<char> data(reinterpret_cast<char *>(&value), sizeof(value));
  return read(data);
}

int Deserializer::read_int64(int64_t &value)
{
  span<char> data(reinterpret_cast<char *>(&value), sizeof(value));
  return read(data);
}

}  // namespace common
