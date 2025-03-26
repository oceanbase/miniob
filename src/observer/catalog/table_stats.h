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

/**
 * @class TableStats
 * @brief Represents statistics related to a table.
 *
 * The TableStats class holds statistical information about a table,
 * such as the number of rows it contains.
 * TODO: Add more statistics as needed.
 */
class TableStats
{
public:
  explicit TableStats(int row_nums) : row_nums(row_nums) {}

  TableStats() = default;

  TableStats(const TableStats &other) { row_nums = other.row_nums; }

  TableStats &operator=(const TableStats &other)
  {
    row_nums = other.row_nums;
    return *this;
  }

  ~TableStats() = default;

  int row_nums = 0;
};