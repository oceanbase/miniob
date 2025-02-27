/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// An iterator yields a sequence of key/value pairs from a source.
// The following class defines the interface.  Multiple implementations
// are provided by this library.  In particular, iterators are provided
// to access the contents of a Table or a DB.
//
// Multiple threads can invoke const methods on an ObLsmIterator without
// external synchronization, but if any of the threads may call a
// non-const method, all threads accessing the same ObLsmIterator must use
// external synchronization.

#pragma once

#include "common/lang/string_view.h"
#include "common/sys/rc.h"

namespace oceanbase {

/**
 * @class ObLsmIterator
 * @brief Abstract class for iterating over key-value pairs in an LSM-Tree.
 *
 * This class provides an interface for iterators used to traverse key-value entries
 * stored in an LSM-Tree. Derived classes must implement this interface to handle
 * specific storage structures, such as SSTables or MemTables.
 */
class ObLsmIterator
{
public:
  ObLsmIterator(){};

  ObLsmIterator(const ObLsmIterator &) = delete;

  ObLsmIterator &operator=(const ObLsmIterator &) = delete;

  virtual ~ObLsmIterator(){};

  /**
   * @brief Checks if the iterator is currently positioned at a valid key-value pair.
   *
   * @return `true` if the iterator is valid, `false` otherwise.
   */
  virtual bool valid() const = 0;

  /**
   * @brief Moves the iterator to the next key-value pair in the source.
   */
  virtual void next() = 0;

  /**
   * @brief Returns the key of the current entry the iterator is positioned at.
   *
   * This method retrieves the key corresponding to the key-value pair at the
   * current position of the iterator.
   *
   * @return A `string_view` containing the key of the current entry.
   */
  virtual string_view key() const = 0;

  /**
   * @brief Returns the value of the current entry the iterator is positioned at.
   *
   * This method retrieves the value corresponding to the key-value pair at the
   * current position of the iterator.
   *
   * @return A `string_view` containing the value of the current entry.
   */
  virtual string_view value() const = 0;

  /**
   * @brief Positions the iterator at the first entry with a key greater than or equal to the specified key.
   *
   * @param k The key to search for.
   */
  virtual void seek(const string_view &k) = 0;

  /**
   * @brief Positions the iterator at the first key-value pair in the source.
   *
   */
  virtual void seek_to_first() = 0;

  /**
   * @brief Positions the iterator at the last key-value pair in the source.
   *
   */
  virtual void seek_to_last() = 0;
};

}  // namespace oceanbase
