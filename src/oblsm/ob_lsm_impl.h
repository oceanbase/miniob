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

#include "oblsm/include/ob_lsm.h"

#include "common/lang/mutex.h"
#include "common/lang/atomic.h"
#include "common/lang/memory.h"
#include "common/lang/filesystem.h"
#include "common/lang/condition_variable.h"
#include "common/thread/thread_pool_executor.h"
#include "oblsm/memtable/ob_memtable.h"
#include "oblsm/table/ob_sstable.h"
#include "oblsm/util/ob_lru_cache.h"
#include "oblsm/compaction/ob_compaction.h"
#include "oblsm/ob_manifest.h"
#include <cstdint>
#include <stdexcept>

namespace oceanbase {

class ObLsmImpl : public ObLsm
{
public:
  ObLsmImpl(const ObLsmOptions &options, const string &path);
  ~ObLsmImpl()
  {
    executor_.shutdown();
    executor_.await_termination();
  }

  RC put(const string_view &key, const string_view &value) override;

  RC get(const string_view &key, string *value) override;

  ObLsmIterator *new_iterator(ObLsmReadOptions options) override;

  SSTablesPtr get_sstables() { return sstables_; }

  // used for debug
  void dump_sstables() override;

private:
  /**
   * @brief Attempts to freeze the current active MemTable.
   *
   * This method performs operations to freeze the active MemTable when certain conditions
   * are met, such as size thresholds or timing requirements. A frozen MemTable becomes
   * immutable and is ready for compaction.
   *
   * @return RC Status code indicating the success or failure of the freeze operation.
   */
  RC try_freeze_memtable();

  /**
   * @brief Performs compaction on the SSTables selected by the compaction strategy.
   *
   * This function takes a compaction plan (represented by `ObCompaction`) and merges
   * the inputs into a new set of SSTables. It creates iterators for the SSTables being
   * compacted, merges their data, and writes the merged data into new SSTable files.
   *
   * @param picked A pointer to the compaction plan that specifies the input SSTables to merge.
   *               If `picked` is `nullptr`, no compaction is performed and an empty result is returned.
   *
   * @return vector<shared_ptr<ObSSTable>> A vector of shared pointers to the newly created SSTables
   *                                       resulting from the compaction process.
   *
   * @details
   * - The function retrieves the inputs (SSTables) from the `picked` compaction plan.
   * - For each SSTable, it creates a new iterator to sequentially scan its data.
   * - It merges the iterators using a merging iterator (`ObLsmIterator`).
   * - It writes the merged key-value pairs into new SSTable files using `ObSSTableBuilder`.
   * - If the size of the new SSTable exceeds a predefined size (`options_.table_size`),
   *   the builder finalizes the current SSTable and starts a new one.
   *
   * @warning Ensure that the `picked` object is properly populated with valid inputs.
   *
   */
  vector<shared_ptr<ObSSTable>> do_compaction(ObCompaction *compaction);

  /**
   * @brief Initiates a major compaction process.
   *
   * Major compaction involves merging all levels of SSTables into a single, consolidated
   * SSTable, which reduces storage fragmentation and improves read performance.
   * This process typically runs periodically or when triggered by specific conditions.
   *
   * @note This function should be called with care, as major compaction is a resource-intensive
   *       operation and may affect system performance during execution.
   */
  void try_major_compaction();

  /**
   * @brief Handles background compaction tasks.
   */
  void background_compaction();

  /**
   * @brief Builds an SSTable from the given MemTable.
   *
   * Converts the data in an immutable MemTable (`imem`) into a new SSTable and writes
   * it to persistent storage. This step is usually part of the compaction pipeline.
   *
   * @param imem A shared pointer to the immutable MemTable (`ObMemTable`) to be converted
   *             into an SSTable.
   * @note The caller must ensure that `imem` is immutable and ready for conversion.
   */
  void build_sstable(shared_ptr<ObMemTable> imem);

  /**
   * @brief Retrieves the file path for a given SSTable.
   *
   * @param sstable_id The unique identifier of the SSTable whose path needs to be retrieved.
   * @return A string representing the full file path of the SSTable.
   */
  string get_sstable_path(uint64_t sstable_id);

  ObLsmOptions                                               options_;
  string                                                     path_;
  mutex                                                      mu_;
  shared_ptr<ObMemTable>                                     mem_table_;
  vector<shared_ptr<ObMemTable>>                             imem_tables_;
  SSTablesPtr                                                sstables_;
  common::ThreadPoolExecutor                                 executor_;
  atomic<uint64_t>                                           seq_{0};
  atomic<uint64_t>                                           sstable_id_{0};
  condition_variable                                         cv_;
  const ObDefaultComparator                                  default_comparator_;
  atomic<bool>                                               compacting_ = false;
  std::unique_ptr<ObLRUCache<uint64_t, shared_ptr<ObBlock>>> block_cache_;
};

}  // namespace oceanbase