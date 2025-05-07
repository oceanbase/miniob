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

#include "common/sys/rc.h"
#include "common/lang/string_view.h"
#include "common/lang/string.h"
#include "oblsm/include/ob_lsm_iterator.h"
#include "oblsm/include/ob_lsm_options.h"
#include "oblsm/include/ob_lsm.h"
#include "oblsm/memtable/ob_skiplist.h"
#include "oblsm/util/ob_arena.h"
#include "oblsm/util/ob_coding.h"
#include "common/lang/map.h"

namespace oceanbase {

/**
 * @class ObLsmTransaction
 * @brief A class representing a transaction in oblsm.
 *
 * The `ObLsmTransaction` class provides functionality for transactional operations
 * on oblsm. It enables reading, writing, deleting, and iterating
 * over keys/values in the database within a transactional scope. Transactions can be
 * committed or rollback, ensuring atomicity and isolation.
 */
class ObLsmTransaction
{
public:
  /**
   * @brief Constructor.
   *
   * Initializes a transaction object and associates it with a specific LSM database.
   * Assigns a unique timestamp (`ts`) to the transaction.
   *
   * @param db A pointer to the `ObLsm` database on which this transaction operates.
   * @param ts The timestamp for the transaction.
   */
  ObLsmTransaction(ObLsm *db, uint64_t ts);

  ~ObLsmTransaction() = default;

  /*
   * @brief Retrieves the value associated with a given key.
   *
   * @param key The key to look up within the database.
   * @param value A pointer to a string where the associated value will be stored, if found.
   */
  RC get(const string_view &key, string *value);

  /**
   * @brief Adds or updates a key-value pair within the transaction's in-memory store.
   */
  RC put(const string_view &key, const string_view &value);

  /**
   * @brief Removes a key from the database within the transaction's scope.
   *
   * This method marks the specified key for removal, but it is not persisted to the database until the transaction is
   * committed.
   */
  RC remove(const string_view &key);

  /**
   * The iterator allows traversal of keys and values within the database. Options can define
   * how data is accessed, such as timestamp.
   *
   * @param options The `ObLsmReadOptions` that define the read behavior of the iterator.
   * @return A pointer to the newly created `ObLsmIterator` object.
   */
  ObLsmIterator *new_iterator(ObLsmReadOptions options);

  /**
   * @brief Commits the transaction, persisting all transaction changes to the database.
   *
   */
  RC commit();

  /**
   * @brief Rollback the transaction, discarding all uncommitted changes.
   */
  RC rollback();

private:
  /**
   * @brief Pointer to the associated `ObLsm` database.
   *
   * This member variable links the transaction to its underlying database, ensuring that
   * all transactional operations target the correct storage layer.
   */
  ObLsm *db_ = nullptr;

  /**
   * @brief The transaction's unique timestamp.
   *
   */
  uint64_t ts_ = 0;

  /**
   * @brief In-memory store for transactional changes.
   *
   * This map holds key-value pairs that have been inserted or removed within the
   * transaction scope but not yet committed to the database. It's used to track changes
   * and ensure atomicity during commit operations.
   */
  map<string, string> inner_store_;
};

/**
 * @class TrxInnerMapIterator
 * @brief An iterator for traversing the transaction's in-memory store
 */
class TrxInnerMapIterator : public ObLsmIterator
{};
}  // namespace oceanbase