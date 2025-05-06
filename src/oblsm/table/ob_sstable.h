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

#include "oblsm/util/ob_file_reader.h"
#include "common/lang/memory.h"
#include "common/sys/rc.h"
#include "oblsm/table/ob_block.h"
#include "oblsm/util/ob_comparator.h"
#include "oblsm/util/ob_lru_cache.h"

namespace oceanbase {
// TODO: add a dumptool to dump sst files(example for usage: ./dumptool sst_file)
//    ┌─────────────────┐
//    │    block 1      │◄──┐
//    ├─────────────────┤   │
//    │    block 2      │   │
//    ├─────────────────┤   │
//    │      ..         │   │
//    ├─────────────────┤   │
//    │    block n      │◄┐ │
//    ├─────────────────┤ │ │
// ┌─►│  meta size(n)   │ │ │
// │  ├─────────────────┤ │ │
// │  │block meta 1 size│ │ │
// │  ├─────────────────┤ │ │
// │  │  block meta 1   ┼─┼─┘
// │  ├─────────────────┤ │
// │  │      ..         │ │
// │  ├─────────────────┤ │
// │  │block meta n size│ │
// │  ├─────────────────┤ │
// │  │  block meta n   ┼─┘
// │  ├─────────────────┤
// └──┼                 │
//    └─────────────────┘

/**
 * @class ObSSTable
 * @brief Represents an SSTable (Sorted String Table) in the LSM-Tree.
 *
 * The `ObSSTable` class is responsible for managing on-disk sorted string tables (SSTables).
 * It provides methods for initialization, key-value lookups, block reading (with caching support),
 * and creating iterators for traversal. Each SSTable is uniquely identified by an `sst_id_` and
 * interacts with the LRU cache for efficient block access.
 */
class ObSSTable : public enable_shared_from_this<ObSSTable>
{
public:
  /**
   * @brief Constructor for ObSSTable.
   *
   * Initializes an SSTable with its unique ID, file name, comparator, and block cache.
   *
   * @param sst_id A unique identifier for the SSTable.
   * @param file_name The name of the file storing the SSTable data.
   * @param comparator A pointer to the comparator used for key comparison.
   * @param block_cache A pointer to the LRU block cache for caching block-level data.
   */
  ObSSTable(uint32_t sst_id, const string &file_name, const ObComparator *comparator,
      ObLRUCache<uint64_t, shared_ptr<ObBlock>> *block_cache)
      : sst_id_(sst_id),
        file_name_(file_name),
        comparator_(comparator),
        file_reader_(nullptr),
        block_cache_(block_cache)
  {
    (void)block_cache_;
  }

  ~ObSSTable() = default;

  /**
   * @brief Initializes the SSTable instance.
   *
   * This function is responsible for performing setup tasks required for the SSTable,
   * such as preparing file readers or pre-loading block_metas_.
   *
   * @warning This function must be called before performing any operations on the SSTable.
   */
  void init();

  uint32_t sst_id() const { return sst_id_; }

  shared_ptr<ObSSTable> get_shared_ptr() { return shared_from_this(); }

  ObLsmIterator *new_iterator();

  /**
   * @brief Reads a block from the SSTable using the block cache.
   *
   * Attempts to read the specified block using the block cache. If the block is not
   * in the cache, it will load the block from the SSTable file and update the cache.
   *
   * @param block_idx The index of the block to read.
   *
   * @return shared_ptr<ObBlock> A shared pointer to the requested block.
   */
  shared_ptr<ObBlock> read_block_with_cache(uint32_t block_idx) const;

  /**
   * @brief Reads a block directly from the SSTable file.
   *
   * This function bypasses the block cache and directly reads the requested block
   * from the SSTable file.
   *
   * @param block_idx The index of the block to read.
   *
   * @return shared_ptr<ObBlock> A shared pointer to the requested block.
   */
  shared_ptr<ObBlock> read_block(uint32_t block_idx) const;

  uint32_t block_count() const { return block_metas_.size(); }

  uint32_t size() const { return file_reader_->file_size(); }

  const BlockMeta block_meta(int i) const { return block_metas_[i]; }

  const ObComparator *comparator() const { return comparator_; }

  void   remove();
  string first_key() const { return block_metas_.empty() ? "" : block_metas_[0].first_key_; }
  string last_key() const { return block_metas_.empty() ? "" : block_metas_.back().last_key_; }

private:
  uint32_t                 sst_id_;
  string                   file_name_;
  const ObComparator      *comparator_ = nullptr;
  unique_ptr<ObFileReader> file_reader_;
  vector<BlockMeta>        block_metas_;

  ObLRUCache<uint64_t, shared_ptr<ObBlock>> *block_cache_;
};

class TableIterator : public ObLsmIterator
{
public:
  TableIterator(const shared_ptr<ObSSTable> &sst) : sst_(sst), block_cnt_(sst->block_count()) {}
  ~TableIterator() override = default;

  void        seek(const string_view &key) override;
  void        seek_to_first() override;
  void        seek_to_last() override;
  void        next() override;
  bool        valid() const override { return block_iterator_ != nullptr && block_iterator_->valid(); }
  string_view key() const override { return block_iterator_->key(); }
  string_view value() const override { return block_iterator_->value(); }

private:
  void read_block_with_cache();

  const shared_ptr<ObSSTable> sst_;
  uint32_t                    block_cnt_      = 0;
  uint32_t                    curr_block_idx_ = 0;
  shared_ptr<ObBlock>         block_;
  unique_ptr<ObLsmIterator>   block_iterator_;
};

using SSTablesPtr = shared_ptr<vector<vector<shared_ptr<ObSSTable>>>>;

}  // namespace oceanbase
