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

#include <cstddef>
#include <cstdint>
#include "oblsm/ob_lsm_define.h"

namespace oceanbase {

/**
 * @brief Configuration options for the LSM-Tree implementation.
 */
struct ObLsmOptions
{
  ObLsmOptions(){};

  // TODO: all params are used for test, need to reset to appropriate values.
  size_t memtable_size = 8 * 1024;
  // sstable size
  size_t table_size = 16 * 1024;

  // leveled compaction
  size_t default_levels        = 7;
  size_t default_l1_level_size = 128 * 1024;
  size_t default_level_ratio   = 10;
  size_t default_l0_file_num   = 3;

  // tired compaction
  size_t default_run_num = 7;

  // default compaction type
  CompactionType type = CompactionType::LEVELED;

  // it is used to control whether the WAL is forced to be written to the disk every time a new key is written.
  bool force_sync_new_log = true;
};

// TODO: UNIMPLEMENTED
struct ObLsmReadOptions
{
  ObLsmReadOptions(){};

  int64_t seq = -1;
};

}  // namespace oceanbase
