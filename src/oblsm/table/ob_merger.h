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

#include "common/lang/vector.h"
#include "common/lang/memory.h"

namespace oceanbase {

class ObComparator;
class ObLsmIterator;

/**
 * @brief Return an iterator that provided the union of the data in
 * children. For example, an iterator that provided
 * the union of memtable and sstable.
 *
 */
ObLsmIterator *new_merging_iterator(const ObComparator *comparator, vector<unique_ptr<ObLsmIterator>> &&children);

}  // namespace oceanbase
