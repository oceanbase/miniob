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

#include <cmath>
#include "common/lang/span.h"
#include "common/lang/string.h"
#include "common/lang/vector.h"
#include "common/log/log.h"
#include "common/sys/rc.h"
#include "common/value.h"
#include <cstring>
#include <algorithm>

using byte_t    = unsigned char;
using bytes     = vector<byte_t>;
using float64_t = double_t;

// reference: https://github.com/code0xff/orderedcodepp
class OrderedCode
{
public:
  static const byte_t term[];
  static const byte_t lit00[];
  static const byte_t litff[];
  static const byte_t inf[];
  static const byte_t msb[];

  static const byte_t increasing = 0x00;
  static const byte_t decreasing = 0xff;

  struct infinity
  {
    bool operator==(const infinity &i) const { return true; }

    bool operator==(infinity &&i) { return true; }
  };

  template <typename T>
  struct decr
  {
    T val;

    bool operator==(const decr<T> &o) const { return val == o.val; }

    bool operator==(decr<T> o) { return val == o.val; }
  };

  struct string_or_infinity
  {
    string s;
    bool   inf;
  };

  struct trailing_string : string
  {};

  static void invert(span<byte_t> &s)
  {
    std::for_each(s.begin(), s.end(), [](byte_t &c) { c ^= 0xff; });
  }

  static RC append(bytes &s, uint64_t x)
  {
    vector<byte_t> buf(9);
    auto           i = 8;
    for (; x > 0; x >>= 8) {
      buf[i--] = static_cast<byte_t>(x);
    }
    buf[i] = static_cast<byte_t>(8 - i);
    s.insert(s.end(), buf.begin() + i, buf.end());
    return RC::SUCCESS;
  }

  static RC append(bytes &s, int64_t x)
  {
    if (x >= -64 && x < 64) {
      s.insert(s.end(), static_cast<byte_t>(x ^ 0x80));
      return RC::SUCCESS;
    }
    bool neg = x < 0;
    if (neg) {
      x = ~x;
    }
    auto  n = 1;
    bytes buf(10);
    auto  i = 9;
    for (; x > 0; x >>= 8) {
      buf[i--] = static_cast<byte_t>(x);
      n++;
    }
    bool lfb = n > 7;
    if (lfb) {
      n -= 7;
    }
    if (buf[i + 1] < 1 << (8 - n)) {
      n--;
      i++;
    }
    buf[i] |= msb[n];
    if (lfb) {
      buf[--i] = 0xff;
    }
    if (neg) {
      span<byte_t> sp(&buf[i], buf.size() - i);
      invert(sp);
    }
    s.insert(s.end(), buf.begin() + i, buf.end());
    return RC::SUCCESS;
  }

  static RC append(bytes &s, float64_t x)
  {
    RC rc = RC::SUCCESS;
    if (std::isnan(x)) {
      LOG_WARN("append: NaN");
      return RC::INVALID_ARGUMENT;
    }
    uint64_t b;
    memcpy(&b, &x, sizeof(x));
    auto i = int64_t(b);
    if (i < 0) {
      i = std::numeric_limits<int64_t>::min() - i;
    }
    if (OB_FAIL(append(s, i))) {
      LOG_WARN("append: append failed, i=%ld, x=%lf", i, x);
      return rc;
    }
    return rc;
  }

  static RC append(bytes &s, const string &x)
  {
    auto l = x.begin();
    for (auto c = x.begin(); c < x.end(); c++) {
      switch (byte_t(*c)) {
        case 0x00:
          s.insert(s.end(), l, c);
          s.insert(s.end(), &lit00[0], &lit00[0] + 2);
          l = c + 1;
          break;
        case 0xff:
          s.insert(s.end(), l, c);
          s.insert(s.end(), &litff[0], &litff[0] + 2);
          l = c + 1;
      }
    }
    s.insert(s.end(), l, x.end());
    s.insert(s.end(), &term[0], &term[0] + 2);
    return RC::SUCCESS;
  }

  static RC append(bytes &s, const trailing_string &x)
  {
    s.insert(s.end(), x.begin(), x.end());
    return RC::SUCCESS;
  }

  static RC append(bytes &s, const infinity &_)
  {
    s.insert(s.end(), &inf[0], &inf[0] + 2);
    return RC::SUCCESS;
  }

  static RC append(bytes &s, const string_or_infinity &x)
  {
    RC rc = RC::SUCCESS;
    if (x.inf) {
      if (!x.s.empty()) {
        LOG_WARN("orderedcode: string_or_infinity has non-zero string and non-zero infinity");
        return RC::INVALID_ARGUMENT;
      }
      if (OB_FAIL(append(s, infinity{}))) {
        LOG_WARN("orderedcode: append infinity failed");
        return rc;
      }
    } else {
      if (OB_FAIL(append(s, x.s))) {
        LOG_WARN("orderedcode: append string failed");
        return rc;
      }
    }
    return rc;
  }

  static RC parse(span<byte_t> &s, byte_t dir, int64_t &dst)
  {
    if (s.empty()) {
      LOG_WARN("orderedcode: corrupt input");
      return RC::INVALID_ARGUMENT;
    }
    byte_t c = s[0] ^ dir;
    if (c >= 0x40 && c < 0xc0) {
      dst = int64_t(int8_t(c ^ 0x80));
      s   = s.subspan(1);
      return RC::SUCCESS;
    }
    bool neg = (c & 0x80) == 0;
    if (neg) {
      c   = ~c;
      dir = ~dir;
    }
    size_t n = 0;
    if (c == 0xff) {
      if (s.size() == 1) {
        LOG_WARN("orderedcode: corrupt input");
        return RC::INVALID_ARGUMENT;
      }
      s = s.subspan(1);
      c = s[0] ^ dir;
      if (c > 0xc0) {
        LOG_WARN("orderedcode: corrupt input");
        return RC::INVALID_ARGUMENT;
      }
      n = 7;
    }
    for (byte_t mask = 0x80; (c & mask) != 0; mask >>= 1) {
      c &= ~mask;
      n++;
    }
    if (s.size() < n) {
      LOG_WARN("orderedcode: corrupt input");
      return RC::INVALID_ARGUMENT;
    }
    int64_t x = c;
    for (size_t i = 1; i < n; i++) {
      c = s[i] ^ dir;
      x = x << 8 | c;
    }
    if (neg) {
      x = ~x;
    }
    dst = x;
    s   = s.subspan(n);
    return RC::SUCCESS;
  }

  static RC parse(span<byte_t> &s, byte_t dir, uint64_t &dst)
  {
    RC rc = RC::SUCCESS;
    if (s.empty()) {
      LOG_WARN("orderedcode: corrupt input");
      return RC::INVALID_ARGUMENT;
    }
    byte_t n = s[0] ^ dir;
    if (n > 8 || (int)s.size() < (1 + n)) {
      LOG_WARN("orderedcode: corrupt input");
      return RC::INVALID_ARGUMENT;
    }
    uint64_t x = 0;
    for (size_t i = 0; i < n; i++) {
      x = x << 8 | (s[1 + i] ^ dir);
    }
    dst = x;
    s   = s.subspan(1 + n);
    return rc;
  }

  static RC parse(span<byte_t> &s, byte_t dir, infinity &_)
  {
    RC rc = RC::SUCCESS;
    if (s.size() < 2) {
      LOG_WARN("orderedcode: corrupt input");
      return RC::INVALID_ARGUMENT;
    }
    if ((s[0] ^ dir) != inf[0] || (s[1] ^ dir) != inf[1]) {
      LOG_WARN("orderedcode: corrupt input");
      return RC::INVALID_ARGUMENT;
    }
    s = s.subspan(2);
    return rc;
  }

  static RC parse(span<byte_t> &s, byte_t dir, string &dst)
  {
    bytes buf;
    for (auto l = 0, i = 0; i < (int)s.size();) {
      switch (s[i] ^ dir) {
        case 0x00:
          if (i + 1 >= (int)s.size()) {
            LOG_WARN("orderedcode: corrupt input");
            return RC::INVALID_ARGUMENT;
          }
          switch (s[i + 1] ^ dir) {
            case 0x01:
              dst.clear();
              if (l == 0 && dir == increasing) {
                dst.insert(dst.end(), s.begin(), s.begin() + i);
                s = s.subspan(i + 2);
                return RC::SUCCESS;
              }
              buf.insert(buf.end(), s.begin() + l, s.begin() + i);
              if (dir == decreasing) {
                span<byte_t> sp(buf);
                invert(sp);
              }
              dst.insert(dst.end(), buf.begin(), buf.end());
              s = s.subspan(i + 2);
              return RC::SUCCESS;
            case 0xff:
              buf.insert(buf.end(), s.begin() + l, s.begin() + i);
              buf.insert(buf.end(), static_cast<byte_t>(0x00 ^ dir));
              i += 2;
              l = i;
              break;
            default: LOG_WARN("orderedcode: corrupt input"); return RC::INVALID_ARGUMENT;
          }
          break;
        case 0xff:
          if (i + 1 >= (int)s.size() || ((s[i + 1] ^ dir) != 0x00)) {
            LOG_WARN("orderedcode: corrupt input");
            return RC::INVALID_ARGUMENT;
          }
          buf.insert(buf.end(), s.begin() + l, s.begin() + i);
          buf.insert(buf.end(), static_cast<byte_t>(0xff ^ dir));
          i += 2;
          l = i;
          break;
        default: i++;
      }
    }
    LOG_WARN("orderedcode: corrupt input");
    return RC::INVALID_ARGUMENT;
  }

  static RC parse(span<byte_t> &s, byte_t dir, float64_t &dst)
  {
    RC      rc = RC::SUCCESS;
    int64_t i  = 0;
    parse(s, dir, i);
    if (i < 0) {
      i = ((int64_t)-1 << 63) - i;
    }
    memcpy(&dst, &i, sizeof(i));
    if (std::isnan(dst)) {
      rc = RC::INVALID_ARGUMENT;
    }
    return rc;
  }

  static RC parse(span<byte_t> &s, byte_t dir, string_or_infinity &dst)
  {
    RC rc = RC::SUCCESS;
    try {
      infinity _;
      rc      = parse(s, dir, _);
      dst.inf = true;
      return rc;
    } catch (...) {
      rc = parse(s, dir, dst.s);
      return rc;
    }
  }

  static RC parse(span<byte_t> &s, byte_t dir, trailing_string &dst)
  {
    dst.clear();
    if (dir == increasing) {
      dst.insert(dst.end(), s.begin(), s.end());
    } else {
      invert(s);
      dst.insert(dst.end(), s.begin(), s.end());
    }
    return RC::SUCCESS;
  }
};

class Codec
{
public:
  static RC encode_without_rid(int64_t table_id, bytes &encoded_key)
  {
    RC rc = RC::SUCCESS;
    if (OB_FAIL(OrderedCode::append(encoded_key, table_prefix))) {
      LOG_WARN("append failed");
    } else if (OB_FAIL(OrderedCode::append(encoded_key, table_id))) {
      LOG_WARN("append failed");
    }
    return rc;
  }
  static RC encode(int64_t table_id, uint64_t rid, bytes &encoded_key)
  {
    RC rc = RC::SUCCESS;
    if (OB_FAIL(OrderedCode::append(encoded_key, table_prefix))) {
      LOG_WARN("append failed");
    } else if (OB_FAIL(OrderedCode::append(encoded_key, table_id))) {
      LOG_WARN("append failed");
    } else if (OB_FAIL(OrderedCode::append(encoded_key, rowkey_prefix))) {
      LOG_WARN("append failed");
    } else if (OB_FAIL(OrderedCode::append(encoded_key, rid))) {
      LOG_WARN("append failed");
    }
    return rc;
  }

  static RC encode_table_prefix(int64_t table_id, bytes &encoded_key)
  {
    RC rc = RC::SUCCESS;
    if (OB_FAIL(OrderedCode::append(encoded_key, table_prefix))) {
      LOG_WARN("append failed");
    } else if (OB_FAIL(OrderedCode::append(encoded_key, table_id))) {
      LOG_WARN("append failed");
    } else if (OB_FAIL(OrderedCode::append(encoded_key, rowkey_prefix))) {
      LOG_WARN("append failed");
    }
    return rc;
  }

  static RC encode_value(const Value &val, bytes &dst)
  {
    RC rc = RC::SUCCESS;
    switch (val.attr_type()) {
      case AttrType::INTS:
        if (OB_FAIL(OrderedCode::append(dst, (int64_t)val.get_int()))) {
          LOG_WARN("append failed");
        }
        break;
      case AttrType::FLOATS:
        if (OB_FAIL(OrderedCode::append(dst, (double)val.get_float()))) {
          LOG_WARN("append failed");
        }
        break;
      case AttrType::CHARS:
        if (OB_FAIL(OrderedCode::append(dst, val.get_string()))) {
          LOG_WARN("append failed");
        }
        break;
      default: return RC::INVALID_ARGUMENT;
    }
    return rc;
  }

  static RC encode_int(int64_t val, bytes &dst)
  {
    RC rc = RC::SUCCESS;
    if (OB_FAIL(OrderedCode::append(dst, val))) {
      LOG_WARN("append failed");
    }
    return rc;
  }

  static RC decode(bytes &encoded_key, int64_t &table_id)
  {
    RC           rc = RC::SUCCESS;
    span<byte_t> sp(encoded_key);
    string       table_prefix;
    if (OB_FAIL(OrderedCode::parse(sp, OrderedCode::increasing, table_prefix))) {
      LOG_WARN("parse failed");
      return rc;
    } else if (OB_FAIL(OrderedCode::parse(sp, OrderedCode::increasing, table_id))) {
      LOG_WARN("parse failed");
      return rc;
    }
    return rc;
  }

  static constexpr const char *table_prefix  = "t";
  static constexpr const char *rowkey_prefix = "r";
};

// template<typename T>
// void append(bytes& s, decr<T> d) {
//   size_t n = s.size();
//   append(s, d.val);
//   span<byte_t> sp(&s[n], s.size() - n);
//   invert(sp);
// }

// template<typename It, typename... Its>
// void append(bytes& s, It it, Its... its) {
//   append(s, it);
//   append(s, its...);
// }

// template<typename T>
// RC parse(span<byte_t>& s, decr<T>& dst) {
//   return parse(s, decreasing, dst.val);
// }

// template<typename It>
// RC parse(span<byte_t>& s, It& it) {
//   return parse(s, increasing, it);
// }

// template<typename It, typename... Its>
// RC parse(span<byte_t>& s, It& it, Its&... its) {
//   RC rc = RC::SUCCESS;
//   if (OB_FAIL(parse(s, it))) {
//     LOG_WARN("parse failed");
//     return rc;
//   }
//   if (OB_FAIL(parse(s, its...))) {
//     LOG_WARN("parse failed");
//     return rc;
//   }
//   return rc;
// }
