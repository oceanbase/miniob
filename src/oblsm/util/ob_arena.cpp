// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "oblsm/util/ob_arena.h"

namespace oceanbase {

ObArena::ObArena() : memory_usage_(0) {}

ObArena::~ObArena()
{
  for (size_t i = 0; i < blocks_.size(); i++) {
    delete[] blocks_[i];
  }
}

}  // namespace oceanbase
