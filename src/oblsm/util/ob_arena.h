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

#include <cassert>
#include "common/lang/atomic.h"
#include "common/lang/vector.h"

namespace oceanbase {

/**
 * @brief a simple memory allocator.
 * @todo optimize fractional memory allocation
 * @note 1. alloc memory from arena, no need to free it.
 *       2. not thread-safe.
 */
class ObArena
{
public:
  ObArena();

  ObArena(const ObArena &)            = delete;
  ObArena &operator=(const ObArena &) = delete;

  ~ObArena();

  char *alloc(size_t bytes);

  size_t memory_usage() const { return memory_usage_; }

private:
  // Array of new[] allocated memory blocks
  vector<char *> blocks_;

  // Total memory usage of the arena.
  size_t memory_usage_;
};

inline char *ObArena::alloc(size_t bytes)
{
  if (bytes <= 0) {
    return nullptr;
  }
  char *result = new char[bytes];
  blocks_.push_back(result);
  memory_usage_ += bytes + sizeof(char *);
  return result;
}

}  // namespace oceanbase
