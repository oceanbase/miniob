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

#include <stdint.h>
#include <cstddef>

namespace oceanbase {

/**
 * @class ObLRUCache
 * @brief A thread-safe implementation of an LRU (Least Recently Used) cache.
 *
 * The `ObLRUCache` class provides a fixed-size cache that evicts the least recently used
 * entries when the cache exceeds its capacity. It supports thread-safe operations for
 * inserting, retrieving, and checking the existence of cache entries.
 *
 * @tparam KeyType The type of keys used to identify cache entries.
 * @tparam ValueType The type of values stored in the cache.
 */
template <typename KeyType, typename ValueType>
class ObLRUCache
{
public:
  /**
   * @brief Constructs an `ObLRUCache` with a specified capacity.
   *
   * @param capacity The maximum number of elements the cache can hold.
   */
  ObLRUCache(size_t capacity) : capacity_(capacity) {}

  /**
   * @brief Retrieves a value from the cache using the specified key.
   *
   * This method searches for the specified key in the cache. If the key is found, the
   * corresponding value is returned and the key-value pair is moved to the front of the
   * LRU list (indicating recent use).
   *
   * @param key The key to search for in the cache.
   * @param value A reference to store the value associated with the key.
   * @return `true` if the key is found and the value is retrieved; `false` otherwise.
   */
  bool get(const KeyType &key, ValueType &value) { return false; }

  /**
   * @brief Inserts a key-value pair into the cache.
   *
   * If the key already exists in the cache, its value is updated, and the key-value pair
   * is moved to the front of the LRU list. If the cache exceeds its capacity after insertion,
   * the least recently used entry is evicted.
   *
   * @param key The key to insert into the cache.
   * @param value The value to associate with the specified key.
   */
  void put(const KeyType &key, const ValueType &value) {}

  /**
   * @brief Checks whether the specified key exists in the cache.
   *
   * @param key The key to check in the cache.
   * @return `true` if the key exists; `false` otherwise.
   */
  bool contains(const KeyType &key) const { return false; }

private:
  /**
   * @brief The maximum number of elements the cache can hold.
   */
  size_t capacity_;
};

/**
 * @brief Creates a new instance of `ObLRUCache` with the specified capacity.
 *
 * This factory function constructs an `ObLRUCache` instance for the specified key and
 * value types, and initializes it with the given capacity.
 *
 * @tparam Key The type of keys used to identify cache entries.
 * @tparam Value The type of values stored in the cache.
 * @param capacity The maximum number of elements the cache can hold.
 * @return A pointer to the newly created `ObLRUCache` instance.
 */
template <typename Key, typename Value>
ObLRUCache<Key, Value> *new_lru_cache(uint32_t capacity)
{
  return nullptr;
}

}  // namespace oceanbase
