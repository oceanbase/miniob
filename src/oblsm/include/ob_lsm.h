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
#include "common/lang/string_view.h"
#include "common/sys/rc.h"
#include "common/lang/utility.h"
#include "oblsm/include/ob_lsm_options.h"
#include "oblsm/include/ob_lsm_iterator.h"

namespace oceanbase {

class ObLsmTransaction;
/**
 * @brief ObLsm is a key-value storage engine for educational purpose.
 * ObLsm learned a lot about design from leveldb and streamlined it.
 * TODO: add more comments about ObLsm.
 */
class ObLsm
{
public:
  /**
   * @brief Opens an LSM-Tree database at the specified path.
   *
   * This is a static method that initializes an LSM-Tree database instance.
   * It allocates memory for the database and returns a pointer to it through the
   * `dbptr` parameter. The caller is responsible for freeing the memory allocated
   * for the database by deleting the returned pointer when it is no longer needed.
   *
   * @param options A reference to the LSM-Tree options configuration.
   * @param path A string specifying the path to the database.
   * @param dbptr A double pointer to store the allocated database instance.
   * @return An RC value indicating success or failure of the operation.
   * @note The caller must delete the returned database pointer (`*dbptr`) when done.
   */
  static RC open(const ObLsmOptions &options, const string &path, ObLsm **dbptr);

  ObLsm() = default;

  ObLsm(const ObLsm &) = delete;

  ObLsm &operator=(const ObLsm &) = delete;

  virtual ~ObLsm() = default;

  /**
   * @brief Inserts or updates a key-value entry in the LSM-Tree.
   *
   * This method adds a new entry
   *
   * @param key The key to insert or update.
   * @param value The value associated with the key.
   * @return An RC value indicating success or failure of the operation.
   */
  virtual RC put(const string_view &key, const string_view &value) = 0;

  /**
   * @brief Retrieves the value associated with a specified key.
   *
   * This method looks up the value corresponding to the given key in the LSM-Tree.
   * If the key exists, the value is stored in the output parameter `*value`.
   *
   * @param key The key to look up.
   * @param value Pointer to a string where the retrieved value will be stored.
   * @return An RC value indicating success or failure of the operation.
   */
  virtual RC get(const string_view &key, string *value) = 0;

  /**
   * @brief Delete a key-value entry in the LSM-Tree.
   *
   * This method remove a entry
   *
   * @param key The key to remove.
   * @return An RC value indicating success or failure of the operation.
   */
  virtual RC remove(const string_view &key) = 0;

  // TODO: distinguish transaction interface and non-transaction interface, refer to rocksdb
  virtual ObLsmTransaction *begin_transaction() = 0;

  /**
   * @brief Creates a new iterator for traversing the LSM-Tree database.
   *
   * This method returns a heap-allocated iterator over the contents of the
   * database. The iterator is initially invalid, and the caller must use one
   * of the `seek`/`seek_to_first`/`seek_to_last` methods on the iterator
   * before accessing any elements.
   *
   * @param options Read options to configure the behavior of the iterator.
   * @return A pointer to the newly created iterator.
   * @note The caller is responsible for deleting the iterator when it is no longer needed.
   */
  virtual ObLsmIterator *new_iterator(ObLsmReadOptions options) = 0;

  /**
   * @brief Inserts a batch of key-value entries into the LSM-Tree.
   *
   * @param kvs A vector of key-value pairs to insert.
   * @return An RC value indicating success or failure of the operation.
   */
  virtual RC batch_put(const vector<pair<string, string>> &kvs) = 0;

  /**
   * @brief Dumps all SSTables for debugging purposes.
   *
   * This method outputs the structure and contents of all SSTables in the
   * LSM-Tree for debugging or inspection purposes.
   */
  virtual void dump_sstables() = 0;
};

}  // namespace oceanbase
