/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Wangyunlai on 2024/5/31.
//

#pragma once

#include "sql/expr/tuple.h"

/**
 * @brief 组合的Tuple
 * @ingroup Tuple
 * TODO 单元测试
 */
class CompositeTuple : public Tuple
{
public:
  CompositeTuple()          = default;
  virtual ~CompositeTuple() = default;

  /// @brief 删除默认构造函数
  CompositeTuple(const CompositeTuple &) = delete;
  /// @brief 删除默认赋值函数
  CompositeTuple &operator=(const CompositeTuple &) = delete;

  /// @brief 保留移动构造函数
  CompositeTuple(CompositeTuple &&) = default;
  /// @brief 保留移动赋值函数
  CompositeTuple &operator=(CompositeTuple &&) = default;

  int cell_num() const override;
  RC  cell_at(int index, Value &cell) const override;
  RC  spec_at(int index, TupleCellSpec &spec) const override;
  RC  find_cell(const TupleCellSpec &spec, Value &cell) const override;

  void   add_tuple(std::unique_ptr<Tuple> tuple);
  Tuple &tuple_at(size_t index);

private:
  std::vector<std::unique_ptr<Tuple>> tuples_;
};