#pragma once

#include "storage/common/arena_allocator.h"
#include "common/type/string_t.h"
#include "common/log/log.h"

class VectorBuffer
{
public:
  VectorBuffer() = default;

  string_t add_string(const char *data, int len)
  {
    if (len <= string_t::INLINE_LENGTH) {
      return string_t(data, len);
    }
    auto insert_string = empty_string(len);
    auto insert_pos    = insert_string.get_data_writeable();
    memcpy(insert_pos, data, len);
    return insert_string;
  }

  string_t add_string(string_t data) { return add_string(data.data(), data.size()); }

  string_t empty_string(int len)
  {
    ASSERT(len > string_t::INLINE_LENGTH, "len > string_t::INLINE_LENGTH");
    auto insert_pos = heap.Allocate(len);
    return string_t(insert_pos, len);
  }

private:
  Arena heap;
};