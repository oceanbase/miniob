/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by WangYunlai on 2022/11/17.
//

#pragma once

#include <string>
#include <memory>

#include "sql/expr/tuple.h"
#include "sql/operator/physical_operator.h"

class SqlResult {
public:
  SqlResult() = default;
  ~SqlResult()
  {}

  void set_tuple_schema(const TupleSchema &schema);
  void set_return_code(RC rc)
  {
    return_code_ = rc;
  }
  void set_state_string(const std::string &state_string)
  {
    state_string_ = state_string;
  }

  void set_operator(std::unique_ptr<PhysicalOperator> oper)
  {
    operator_ = std::move(oper);
  }
  bool has_operator() const
  {
    return operator_ != nullptr;
  }
  const TupleSchema &tuple_schema() const
  {
    return tuple_schema_;
  }
  RC return_code() const
  {
    return return_code_;
  }
  const std::string &state_string() const
  {
    return state_string_;
  }

  RC open();
  RC close();
  RC next_tuple(Tuple *&tuple);

private:
  std::unique_ptr<PhysicalOperator> operator_;
  TupleSchema tuple_schema_;
  RC return_code_ = RC::SUCCESS;
  std::string state_string_;
};
