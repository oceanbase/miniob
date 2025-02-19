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

#include "common/lang/string_view.h"

namespace oceanbase {

/**
 * @brief base class of all comparators
 */
class ObComparator
{
public:
  virtual ~ObComparator() = default;

  /**
   * @brief Three-way comparison.
   * @return < 0 iff "a" < "b",
   * @return == 0 iff "a" == "b",
   * @return > 0 iff "a" > "b"
   */
  virtual int compare(const string_view &a, const string_view &b) const = 0;
};

/**
 * @brief comparator with lexicographical order
 */
class ObDefaultComparator : public ObComparator
{
public:
  explicit ObDefaultComparator() = default;
  int compare(const string_view &a, const string_view &b) const override;
};

/**
 * @brief internal key comparator
 * @details internal key: | key_size(8B) | key | sequence_number(8B) |
 */
class ObInternalKeyComparator : public ObComparator
{
public:
  explicit ObInternalKeyComparator() = default;

  int                 compare(const string_view &a, const string_view &b) const override;
  const ObComparator *user_comparator() const { return &default_comparator_; }

private:
  ObDefaultComparator default_comparator_;
};

}  // namespace oceanbase
