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

namespace oceanbase {

class ObComparator;
class ObLsmIterator;

/**
 * @brief Creates a new user iterator wrapping the given LSM iterator.
 *
 * @detail This function takes an existing LSM iterator (`ObLsmIterator`) and wraps it to create
 * a user-level iterator. The new iterator is initialized with a specific sequence number (`seq`),
 * which determines the context or version visibility of the iterator.
 *
 * @param iterator The original `ObLsmIterator` to be wrapped.
 * @param seq The sequence number to associate with the new user iterator.
 *
 * @return A pointer to the newly created `ObLsmIterator` instance that acts as a user iterator.
 *
 * @note The caller is responsible for managing the memory of the returned iterator and
 *       ensuring that it is properly deleted after use to prevent memory leaks.
 *
 * @warning Passing a `nullptr` as the `iterator` parameter will result in undefined behavior.
 *          Ensure that a valid iterator is provided before calling this function.
 */
ObLsmIterator *new_user_iterator(ObLsmIterator *iterator, uint64_t seq);

}  // namespace oceanbase