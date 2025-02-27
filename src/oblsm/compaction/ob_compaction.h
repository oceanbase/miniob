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
#include "oblsm/include/ob_lsm_options.h"

namespace oceanbase {

class ObCompactionPicker;

/**
 * @class ObCompaction
 * @brief Represents a compaction task in the LSM-Tree.
 *
 * This class encapsulates the metadata and operations for a single compaction task,
 * including input SSTables and the target level of the compaction.
 */
class ObCompaction
{
public:
  /**
   * @brief Grants access to private members of this class to compaction picker classes.
   * @see ObCompactionPicker, TiredCompactionPicker, LeveledCompactionPicker
   */
  friend class ObCompactionPicker;
  friend class TiredCompactionPicker;
  friend class LeveledCompactionPicker;

  /**
   * @brief Constructs a compaction task targeting a specific level.
   * @param level The current level of the SSTables involved in the compaction.
   */
  explicit ObCompaction(int level) : level_(level) {}

  ~ObCompaction() = default;

  /**
   * @brief Gets the target level for this compaction.
   * @return The integer value representing the level.
   */
  int level() const { return level_; }

  /**
   * @brief Retrieves an SSTable from the input SSTable list.
   * @param which Index indicating which level's inputs to access (0 for `level_`, 1 for `level_ + 1`).
   * @param i Index of the SSTable within the specified level's inputs.
   * @return A shared pointer to the specified SSTable.
   */
  shared_ptr<ObSSTable> input(int which, int i) const { return inputs_[which][i]; }

  /**
   * @brief Computes the total number of input SSTables for the compaction task.
   * @return The total number of input SSTables across both levels.
   */
  int size() const { return inputs_[0].size() + inputs_[1].size(); }

  /**
   * @brief Retrieves the vector of SSTables from the specified input level.
   */
  const vector<shared_ptr<ObSSTable>> &inputs(int which) const { return inputs_[which]; }

private:
  /// Each compaction reads inputs from "level_" and "level_+1"
  std::vector<shared_ptr<ObSSTable>> inputs_[2];

  /**
   * @brief The current level of SSTables involved in the compaction.
   */
  int level_;
};

}  // namespace oceanbase