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
namespace oceanbase {

static constexpr const char *SSTABLE_SUFFIX  = ".sst";
static constexpr const char *MANIFEST_SUFFIX = ".mf";
static constexpr const char *WAL_SUFFIX      = ".wal";

/**
 * @enum CompactionType
 * @brief Defines the types of compaction strategies in an LSM-Tree or similar systems.
 */
enum class CompactionType
{
  TIRED = 0,
  LEVELED,
  UNKNOWN,
};

}  // namespace oceanbase