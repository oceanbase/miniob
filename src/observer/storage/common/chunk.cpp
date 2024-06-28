/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "storage/common/chunk.h"

void Chunk::add_column(unique_ptr<Column> col, int col_id)
{
  columns_.push_back(std::move(col));
  column_ids_.push_back(col_id);
}

RC Chunk::reference(Chunk &chunk)
{
  reset();
  this->columns_.resize(chunk.column_num());
  for (size_t i = 0; i < columns_.size(); ++i) {
    if (nullptr == columns_[i]) {
      columns_[i] = make_unique<Column>();
    }
    columns_[i]->reference(chunk.column(i));
    column_ids_.push_back(chunk.column_ids(i));
  }
  return RC::SUCCESS;
}

int Chunk::rows() const
{
  if (!columns_.empty()) {
    return columns_[0]->count();
  }
  return 0;
}

int Chunk::capacity() const
{
  if (!columns_.empty()) {
    return columns_[0]->capacity();
  }
  return 0;
}

void Chunk::reset_data()
{
  for (auto &col : columns_) {
    col->reset_data();
  }
}

void Chunk::reset()
{
  columns_.clear();
  column_ids_.clear();
}