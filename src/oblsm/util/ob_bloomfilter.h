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
  ObBloomfilter(size_t hash_func_count = 4, size_t totoal_bits = 65536) {}

  /**
   * @brief Inserts an object into the Bloom filter.
   * @details This method computes hash values for the given object and sets corresponding bits in the filter.
   * @param object The object to be inserted.
   */
  void insert(const string &object) {}

  /**
   * @brief Clears all entries in the Bloom filter.
   *
   * @details Resets the filter, removing all previously inserted objects.
   */
  void clear() {}

  /**
   * @brief Checks if an object is possibly in the Bloom filter.
   *
   * @param object The object to be checked.
   * @return true if the object might be in the filter, false if definitely not.
   */
  bool contains(const string &object) const { return false; }

  /**
   * @brief Returns the count of objects inserted into the Bloom filter.
   */
  size_t object_count() const { return 0; }

  /**
   * @brief Checks if the Bloom filter is empty.
   * @return true if the filter is empty, false otherwise.
   */
  bool empty() const { return 0 == object_count(); }

private:
};

}  // namespace oceanbase
