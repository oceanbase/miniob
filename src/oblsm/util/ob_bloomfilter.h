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
#include <cstddef>
#include <cstring>
#include <memory>
#include <mutex>
#include <shared_mutex>
namespace oceanbase {

/**
 * @class ObBloomfilter
 * @brief A simple Bloom filter implementation(Need to support concurrency).
 */
class ObBloomfilter
{
public:
  static constexpr size_t DEFAULT_HASH_FUNC_COUNT = 4;
  static constexpr size_t DEFAULT_TOTAL_BITS = 65536;

  /**
   * @brief Constructs a Bloom filter with specified parameters.
   *
   * @param hash_func_count Number of hash functions to use. Default is 4.
   * @param totoal_bits Total number of bits in the Bloom filter. Default is 65536.
   */
  ObBloomfilter(size_t hash_func_count = DEFAULT_HASH_FUNC_COUNT, size_t totoal_bits = DEFAULT_TOTAL_BITS) 
    : total_bits_(totoal_bits),
    cnt_element_(0),
    hash_func_count(hash_func_count),
    bitmap_mem_(std::make_unique<char[]>(bytes())) {
      std::memset(bitmap_mem_.get(), 0, bytes());
    }

  /**
   * @brief Inserts an object into the Bloom filter.
   * @details This method computes hash values for the given object and sets corresponding bits in the filter.
   * @param object The object to be inserted.
   */
  void insert(const string &object) {
    std::unique_lock<std::shared_mutex> lock(rw_mutex);
    for (size_t i = 0; i < hash_func_count; ++i) {
      size_t hash_val = hash(object, i);
      size_t index = hash_val % total_bits_;
      set_bit(index);
    }
    ++cnt_element_;
  }

  /**
   * @brief Clears all entries in the Bloom filter.
   *
   * @details Resets the filter, removing all previously inserted objects.
   */
  void clear() {
    std::unique_lock<std::shared_mutex> lock(rw_mutex);
    std::memset(bitmap_mem_.get(), 0, bytes());
    cnt_element_ = 0;
  }

  /**
   * @brief Checks if an object is possibly in the Bloom filter.
   *
   * @param object The object to be checked.
   * @return true if the object might be in the filter, false if definitely not.
   */
  bool contains(const string &object) const { 
    std::shared_lock<std::shared_mutex> lock(rw_mutex);
    for (size_t i = 0; i < hash_func_count; ++i) {
      size_t hash_val = hash(object, i);
      size_t index = hash_val % total_bits_;
      if (!get_bit(index)) {
        return false;
      }
    }
    return true;
  }

  /**
   * @brief Returns the count of objects inserted into the Bloom filter.
   */
  size_t object_count() const { return cnt_element_; }

  /**
   * @brief Checks if the Bloom filter is empty.
   * @return true if the filter is empty, false otherwise.
   */
  bool empty() const { return 0 == object_count(); }

  // 

private:
  // 位图大小
  size_t total_bits_;
  // 插入元素个数计数器
  size_t cnt_element_;
  // 哈希函数个数
  size_t hash_func_count;
  // 位图
  std::unique_ptr<char[]> bitmap_mem_;
  // 读写锁
  mutable std::shared_mutex rw_mutex;

  // 使用单个哈希函数配合不同种子生成不同的哈希值
  size_t hash(const string &object, int seed) const {
    std::hash<string> hasher;
    size_t hash_val = hasher(object);
    hash_val ^= static_cast<size_t>(seed) + 0x9e3779b9 + (hash_val << 6) + (hash_val >> 2);
    return hash_val;
  }

  bool get_bit(size_t index) const {
    size_t byte_index = index / 8;
    size_t bit_offset = index % 8;
    return (bitmap_mem_[byte_index] & (1 << bit_offset)) != 0;
  }

  void set_bit(size_t index) {
    size_t byte_index = index / 8;
    size_t bit_offset = index % 8;
    bitmap_mem_[byte_index] |= (1 << bit_offset);
  }

  void clear_bit(size_t index) {
    size_t byte_index = index / 8;
    size_t bit_offset = index % 8;
    bitmap_mem_[byte_index] &= ~(1 << bit_offset);
  }

  // 声明 totoal_bits 需要使用多少个 bytes
  size_t bytes() const {
    return total_bits_ % 8 == 0 ? total_bits_ / 8 : (total_bits_ / 8 + 1);
  }
};

}  // namespace oceanbase
