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

#include "common/lang/memory.h"
#include "oblsm/table/ob_block_builder.h"
#include "oblsm/memtable/ob_memtable.h"
#include "common/lang/string.h"
#include "oblsm/util/ob_file_writer.h"
#include "oblsm/table/ob_block.h"
#include "oblsm/table/ob_sstable.h"
#include "oblsm/util/ob_lru_cache.h"

namespace oceanbase {

/**
 * @brief Build a SSTable
 */
class ObSSTableBuilder
{
public:
  ObSSTableBuilder(const ObComparator *comparator, ObLRUCache<uint64_t, shared_ptr<ObBlock>> *block_cache)
      : comparator_(comparator), block_cache_(block_cache)
  {}
  ~ObSSTableBuilder() = default;

  /**
   * @brief Builds an SSTable from the provided in-memory table and stores it in a file.
   *
   * This function takes an `ObMemTable` as input, partitions the data into blocks,
   * serializes the blocks, and writes them into an SSTable file.
   *
   * @param mem_table A shared pointer to the `ObMemTable` containing the data to be written into the SSTable.
   * @param file_name The name of the file where the constructed SSTable will be stored.
   * @param sst_id A unique identifier assigned to the created SSTable.
   *
   * @return RC A result code indicating the success or failure of the SSTable creation process.
   *
   */
  RC                    build(shared_ptr<ObMemTable> mem_table, const string &file_name, uint32_t sst_id);
  size_t                file_size() const { return file_size_; }
  shared_ptr<ObSSTable> get_built_table();
  void                  reset();

private:
  void finish_build_block();

  const ObComparator      *comparator_ = nullptr;
  ObBlockBuilder           block_builder_;
  string                   curr_blk_first_key_;
  unique_ptr<ObFileWriter> file_writer_;
  vector<BlockMeta>        block_metas_;
  uint32_t                 curr_offset_ = 0;
  uint32_t                 sst_id_      = 0;
  size_t                   file_size_   = 0;

  ObLRUCache<uint64_t, shared_ptr<ObBlock>> *block_cache_ = nullptr;
};
}  // namespace oceanbase
