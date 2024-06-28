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

#include "common/rc.h"
#include "common/log/log.h"
#include "common/lang/memory.h"
#include "common/lang/vector.h"
#include "storage/common/column.h"

/**
 * @brief A Chunk represents a set of columns.
 */
class Chunk
{
public:
  Chunk()              = default;
  Chunk(const Chunk &) = delete;
  Chunk(Chunk &&)      = delete;

  int column_num() const { return columns_.size(); }

  Column &column(size_t idx)
  {
    ASSERT(idx < columns_.size(), "invalid column index");
    return *columns_[idx];
  }

  Column *column_ptr(size_t idx)
  {
    ASSERT(idx < columns_.size(), "invalid column index");
    return &column(idx);
  }

  int column_ids(size_t i)
  {
    ASSERT(i < column_ids_.size(), "invalid column index");
    return column_ids_[i];
  }

  void add_column(unique_ptr<Column> col, int col_id);

  RC reference(Chunk &chunk);

  /**
   * @brief 获取 Chunk 中的行数
   */
  int rows() const;

  /**
   * @brief 获取 Chunk 的容量
   */
  int capacity() const;

  /**
   * @brief 从 Chunk 中获得指定行指定列的 Value
   * @param col_idx 列索引
   * @param row_idx 行索引
   * @return Value
   * @note 没有检查 col_idx 和 row_idx 是否越界
   *
   */
  Value get_value(int col_idx, int row_idx) const { return columns_[col_idx]->get_value(row_idx); }

  /**
   * @brief 重置 Chunk 中的数据，不会修改 Chunk 的列属性。
   */
  void reset_data();

  void reset();

private:
  vector<unique_ptr<Column>> columns_;
  // TODO: remove it and support multi-tables,
  // `columnd_ids` store the ids of child operator that need to be output
  vector<int> column_ids_;
};