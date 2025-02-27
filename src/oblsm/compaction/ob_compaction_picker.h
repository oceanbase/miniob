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

#include "oblsm/table/ob_sstable.h"
#include "common/sys/rc.h"
#include "oblsm/compaction/ob_compaction.h"
#include "oblsm/util/ob_comparator.h"

namespace oceanbase {

/**
 * @class ObCompactionPicker
 * @brief Abstract base class for compaction picker strategies in an LSM-Tree.
 *
 * This class defines the interface for picking a compaction task from a set of SSTables.
 * Derived classes will implement specific compaction strategies (e.g., tiered or leveled compaction).
 */
class ObCompactionPicker
{
public:
  /**
   * @param options Pointer to the LSM-Tree options configuration.
   */
  ObCompactionPicker(ObLsmOptions *options) : options_(options) {}

  virtual ~ObCompactionPicker() = default;

  /**
   * @brief Pure virtual method to pick a compaction task.
   * @param sstables A pointer to the SSTables available for compaction.
   * @return A unique pointer to the selected compaction task.
   */
  virtual unique_ptr<ObCompaction> pick(SSTablesPtr sstables) = 0;

  /**
   * @brief Static factory method to create a specific compaction picker.
   * @param type The type of compaction strategy (e.g., tiered, leveled).
   * @param options Pointer to the LSM-Tree options configuration.
   * @return A pointer to the created ObCompactionPicker instance.
   */
  static ObCompactionPicker *create(CompactionType type, ObLsmOptions *options);

protected:
  ObLsmOptions *options_;  ///< Pointer to the LSM-Tree options configuration.
};

/**
 * @class TiredCompactionPicker
 * @brief A class implementing the tiered compaction strategy.
 *
 */
class TiredCompactionPicker : public ObCompactionPicker
{
public:
  /**
   * @param options Pointer to the LSM-Tree options configuration.
   */
  TiredCompactionPicker(ObLsmOptions *options) : ObCompactionPicker(options) {}

  ~TiredCompactionPicker() = default;

  /**
   * @brief Implementation of the pick method for tiered compaction.
   */
  unique_ptr<ObCompaction> pick(SSTablesPtr sstables) override;

private:
};

}  // namespace oceanbase