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

#include <iostream>
#include "storage/table/table.h"
#include "storage/field/field_meta.h"

class TupleCellSpec
{
public:
  TupleCellSpec(const char *table_name, const char *field_name, const char *alias = nullptr, const AggrOp aggr = AggrOp::AGGR_NONE);
  TupleCellSpec(const char *alias, const AggrOp aggr = AggrOp::AGGR_NONE);

  const char *table_name() const
  {
    return table_name_.c_str();
  }
  const char *field_name() const
  {
    return field_name_.c_str();
  }
  const char *alias() const
  {
    return alias_.c_str();
  }
  const AggrOp aggr() const
  {
    return aggr_;
  }

  void aggr_to_string(const AggrOp aggr, std::string &aggr_repr)
  {
    switch (aggr)
    {
    case AggrOp::AGGR_AVG:
      aggr_repr =  "AVG";
      break;
    case AggrOp::AGGR_COUNT:
      aggr_repr = "COUNT";
      break;
    case AggrOp::AGGR_MAX:
      aggr_repr =  "MAX";
      break;
    case AggrOp::AGGR_MIN:
      aggr_repr = "MIN";
      break;
    case AggrOp::AGGR_SUM:
      aggr_repr = "SUM";
      break;
    default:
      break;
    }
  }

private:
  std::string table_name_;
  std::string field_name_;
  std::string alias_;
  AggrOp aggr_;
};
