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

#include <bitset>
#include <memory>
#include <functional>
#include <mutex>

namespace oceanbase {

/**
 * @class ObBloomfilter
 * @brief A simple Bloom filter implementation(Need to support concurrency).
 */
class ObBloomfilter
{
public:
  /**
   * @brief Constructs a Bloom filter with specified parameters.
   * 
   * @param hash_func_count Number of hash functions to use. Default is 4.
   * @param totoal_bits Total number of bits in the Bloom filter. Default is 65536.
   */
  ObBloomfilter(size_t hash_func_count = 4, size_t totoal_bits = 65536) : hash_function_count(hash_func_count), object_count_(0) {}

  /**
   * @brief Inserts an object into the Bloom filter.
   * @details This method computes hash values for the given object and sets corresponding bits in the filter.
   * @param object The object to be inserted.
   */
  void insert(const std::string &object)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    for (size_t i = 0; i < hash_function_count; i++) {
      size_t hash_val                                       = hash(object, i);
      bloomfilter_store_[hash_val % bloomfilter_store_size] = true;
    }
    ++object_count_;
  }
  /**
   * @brief Clears all entries in the Bloom filter.
   * 
   * @details Resets the filter, removing all previously inserted objects.
   */
  void clear()
  {
    bloomfilter_store_.reset();
    object_count_ = 0;
  }
  
  /**
   * @brief Checks if an object is possibly in the Bloom filter.
   * 
   * @param object The object to be checked.
   * @return true if the object might be in the filter, false if definitely not.
   */
  bool contains(const std::string &object) const
  {
    std::lock_guard<std::mutex> lock(mutex_);
    for (size_t i = 0; i < hash_function_count; i++) {
      size_t hash_val = hash(object, i);
      if (!bloomfilter_store_[hash_val % bloomfilter_store_size])
        return false;
    }
    return true;
  }
  
   /**
   * @brief Returns the count of objects inserted into the Bloom filter.
   */
  size_t object_count() const { return object_count_; }
  
  /**
   * @brief Checks if the Bloom filter is empty.
   * @return true if the filter is empty, false otherwise.
   */
  bool empty() const { return 0 == object_count(); }

private:
  /// Size of the bloom filter state in bits (2^16).
  static constexpr size_t bloomfilter_store_size = 65536;

  size_t hash(const std::string &val, size_t seed) const
  {
    return std::hash<std::string>()(val) ^ (seed + 0x9e3779b9 + (seed << 6) + (seed >> 2));
  }

  /// Number of hash functions to use when hashing objects.
  const size_t hash_function_count;

  std::bitset<bloomfilter_store_size> bloomfilter_store_;
  size_t                              object_count_;
  mutable std::mutex                          mutex_;
};

}  // namespace oceanbase
