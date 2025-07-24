/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#pragma once

#include <iostream>
#include <cstring>

using namespace std;
struct string_t
{
public:
  static constexpr int INLINE_LENGTH = 12;

  string_t() = default;

  explicit string_t(uint32_t len) { value.inlined.length = len; }

  string_t(const char *data, uint32_t len) { init(data, len); }

  ~string_t() { reset(); }

  void init(const char *data, uint32_t len)
  {
    value.inlined.length = len;
    if (is_inlined()) {
      memset(value.inlined.inlined, 0, INLINE_LENGTH);
      if (size() == 0) {
        return;
      }
      memcpy(value.inlined.inlined, data, size());
    } else {
      value.pointer.ptr = (char *)data;
    }
  }

  void reset()
  {
    if (is_inlined()) {
      memset(value.inlined.inlined, 0, INLINE_LENGTH);
    } else {
      value.pointer.ptr = nullptr;
    }
    value.inlined.length = 0;
  }

  string_t(const char *data) : string_t(data, strlen(data)) {}
  string_t(const string &value) : string_t(value.c_str(), value.size()) {}

  bool is_inlined() const { return size() <= INLINE_LENGTH; }

  const char *data() const { return is_inlined() ? value.inlined.inlined : value.pointer.ptr; }

  char *get_data_writeable() const { return is_inlined() ? (char *)value.inlined.inlined : value.pointer.ptr; }

  int size() const { return value.inlined.length; }

  bool empty() const { return value.inlined.length == 0; }

  string get_string() const { return string(data(), size()); }

  bool operator==(const string_t &r) const
  {
    if (this->size() != r.size()) {
      return false;
    }
    return (memcmp(this->data(), r.data(), this->size()) == 0);
  }

  bool operator!=(const string_t &r) const { return !(*this == r); }

  bool operator>(const string_t &r) const
  {
    const uint32_t left_length  = this->size();
    const uint32_t right_length = r.size();
    const uint32_t min_length   = std::min<uint32_t>(left_length, right_length);

    auto memcmp_res = memcmp(this->data(), r.data(), min_length);
    return memcmp_res > 0 || (memcmp_res == 0 && left_length > right_length);
  }
  bool operator<(const string_t &r) const { return r > *this; }

  struct Inlined
  {
    uint32_t length;
    char     inlined[12];
  };
  union
  {
    struct
    {
      uint32_t length;
      char    *ptr;
    } pointer;
    Inlined inlined;
  } value;
};