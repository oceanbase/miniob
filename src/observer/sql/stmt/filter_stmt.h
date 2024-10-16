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
// Created by Wangyunlai on 2022/5/22.
//

#pragma once

#include "sql/expr/expression.h"
#include "sql/parser/parse_defs.h"
#include "sql/stmt/stmt.h"
#include "sql/stmt/calc_stmt.h"
#include <unordered_map>
#include <vector>
#include "sql/parser/expression_binder.h"

class Db;
class Table;
class FieldMeta;

struct FilterObj
{
  bool  is_attr;
  bool is_value;
  Field field;
  Value value;
  ArithmeticExpr* arith_exper_;
  Expression* arith_bound_exper_;

  void init_attr(const Field &field)
  {
    is_attr     = true;
    is_value    = false;
    this->field = field;
  }

  void init_value(const Value &value)
  {
    is_attr     = false;
    is_value    = true;
    this->value = value;
  }

  void init_arith(unique_ptr<Expression>& abe){
    is_attr = false;
    is_value = false;
    this->arith_bound_exper_ = abe.get();
    abe.release();
  }
};

class FilterUnit
{
public:
  FilterUnit() = default;
  ~FilterUnit() {}

  void set_comp(CompOp comp) { comp_ = comp; }

  CompOp comp() const { return comp_; }

  void set_left(const FilterObj &obj) { 
    
    left_ = obj;
  }
  void set_right(const FilterObj &obj) { right_ = obj; }

  const FilterObj &left() const { return left_; }
  const FilterObj &right() const { return right_; }

private:
  CompOp    comp_ = NO_OP;
  FilterObj left_;
  FilterObj right_;
};

/**
 * @brief Filter/谓词/过滤语句
 * @ingroup Statement
 */
class FilterStmt
{
public:
  FilterStmt() = default;
  virtual ~FilterStmt();

public:
  const std::vector<FilterUnit *> &filter_units() const { return filter_units_; }

public:
  static RC create(Db *db, Table *default_table, std::unordered_map<std::string, Table *> *tables,
      const ConditionSqlNode *conditions, int condition_num, FilterStmt *&stmt, ExpressionBinder& expression_binder);

  static RC create_filter_unit(Db *db, Table *default_table, std::unordered_map<std::string, Table *> *tables,
      const ConditionSqlNode &condition, FilterUnit *&filter_unit, ExpressionBinder& expression_binder);

private:
  std::vector<FilterUnit *> filter_units_;  // 默认当前都是AND关系
};
