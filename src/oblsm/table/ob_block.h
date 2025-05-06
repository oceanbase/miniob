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

#include "common/lang/string.h"
#include "common/lang/vector.h"
#include "oblsm/include/ob_lsm_iterator.h"
#include "oblsm/util/ob_comparator.h"

namespace oceanbase {

// TODO: block align to 4KB
//      ┌─────────────────┐
//      │    entry 1      │◄───┐
//      ├─────────────────┤    │
//      │    entry 2      │    │
//      ├─────────────────┤    │
//      │      ..         │    │
//      ├─────────────────┤    │
//      │    entry n      │◄─┐ │
//      ├─────────────────┤  │ │
// ┌───►│  offset size(n) │  │ │
// │    ├─────────────────┤  │ │
// │    │    offset 1     ├──┼─┘
// │    ├─────────────────┤  │
// │    │      ..         │  │
// │    ├─────────────────┤  │
// │    │    offset n     ├──┘
// │    ├─────────────────┤
// └────┤  offset start   │
//      └─────────────────┘
/**
 * @class ObBlock
 * @brief Represents a data block in the LSM-Tree.
 *
 * The `ObBlock` class manages a block of serialized key-value pairs, along with
 * their offsets, for efficient storage and retrieval. It provides methods to decode
 * serialized data, access individual entries, and create iterators for traversing
 * the block contents.
 */
class ObBlock
{

public:
  ObBlock(const ObComparator *comparator) : comparator_(comparator) {}

  void add_offset(uint32_t offset) { offsets_.push_back(offset); }

  uint32_t get_offset(int index) const { return offsets_[index]; }

  string_view get_entry(uint32_t offset) const;

  int size() const { return offsets_.size(); }

  /**
   * @brief Decodes serialized block data.
   *
   * This function parses and decodes the serialized string data to reconstruct
   * the block's structure, including all key-value offsets and entries.
   * The decoded data format can reference ObBlockBuilder.
   * @param data The serialized block data as a string.
   * @return RC The result code indicating the success or failure of the decode operation.
   */
  RC decode(const string &data);

  ObLsmIterator *new_iterator() const;

private:
  string           data_;
  vector<uint32_t> offsets_;
  // TODO: remove
  const ObComparator *comparator_;
};

class BlockIterator : public ObLsmIterator
{
public:
  BlockIterator(const ObComparator *comparator, const ObBlock *data, uint32_t count)
      : comparator_(comparator), data_(data), count_(count)
  {}
  BlockIterator(const BlockIterator &)            = delete;
  BlockIterator &operator=(const BlockIterator &) = delete;

  ~BlockIterator() override = default;

  void seek(const string_view &lookup_key) override;
  void seek_to_first() override
  {
    index_ = 0;
    parse_entry();
  }
  void seek_to_last() override
  {
    index_ = count_ - 1;
    parse_entry();
  }

  bool valid() const override { return index_ < count_; }
  void next() override
  {
    index_++;
    if (valid()) {
      parse_entry();
    }
  }
  string_view key() const override { return key_; };
  string_view value() const override { return value_; }

private:
  void parse_entry();

private:
  const ObComparator  *comparator_;
  const ObBlock *const data_;
  string_view          curr_entry_;
  string_view          key_;
  string_view          value_;
  uint32_t             count_ = 0;
  uint32_t             index_ = 0;
};

class BlockMeta
{
public:
  BlockMeta() {}
  BlockMeta(const string &first_key, const string &last_key, uint32_t offset, uint32_t size)
      : first_key_(first_key), last_key_(last_key), offset_(offset), size_(size)
  {}
  string encode() const;
  RC     decode(const string &data);

  string first_key_;
  string last_key_;

  // Offset of ObBlock in SSTable
  uint32_t offset_;
  uint32_t size_;
};
}  // namespace oceanbase
