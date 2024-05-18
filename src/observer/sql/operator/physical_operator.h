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
// Created by WangYunlai on 2022/6/7.
//

#pragma once

#include <vector>
#include <memory>
#include <string>

#include "common/rc.h"
#include "sql/expr/tuple.h"

class Record;
class TupleCellSpec;
class Trx;

/**
 * @brief 物理算子
 * @defgroup PhysicalOperator
 * @details 物理算子描述执行计划将如何执行，比如从表中怎么获取数据，如何做投影，怎么做连接等
 */

/**
 * @brief 物理算子类型
 * @ingroup PhysicalOperator
 */
enum class PhysicalOperatorType
{
  TABLE_SCAN,
  INDEX_SCAN,
  NESTED_LOOP_JOIN,
  EXPLAIN,
  PREDICATE,
  PROJECT,
  CALC,
  STRING_LIST,
  DELETE,
  INSERT,
  AGGREGATE,
  UPDATE,
};

/**
 * @brief 与LogicalOperator对应，物理算子描述执行计划将如何执行
 * @ingroup PhysicalOperator
 */
class PhysicalOperator
{
public:
  PhysicalOperator() = default;

  virtual ~PhysicalOperator();

  /**
   * 这两个函数是为了打印时使用的，比如在explain中
   */
  virtual std::string name() const;
  virtual std::string param() const;

  virtual PhysicalOperatorType type() const = 0;

  virtual RC open(Trx *trx) = 0;
  virtual RC next() = 0;
  virtual RC close() = 0;

  virtual Tuple *current_tuple() = 0;

  void add_child(std::unique_ptr<PhysicalOperator> oper)
  {
    children_.emplace_back(std::move(oper));
  }

  std::vector<std::unique_ptr<PhysicalOperator>> &children()
  {
    return children_;
  }

protected:
  std::vector<std::unique_ptr<PhysicalOperator>> children_;
};
