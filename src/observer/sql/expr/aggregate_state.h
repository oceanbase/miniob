/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "sql/expr/expression.h"
#include "common/type/attr_type.h"
template <class T>
class SumState
{
public:
  SumState() : value(0) {}
  T    value;
  void update(const T *values, int size);
  void update(const T &value) { this->value += value; }
  template <class U>
  U finalize()
  {
    return (U)value;
  }
};

template <class T>
class CountState
{
public:
  CountState() : value(0) {}
  int  value;
  void update(const T *values, int size);
  void update(const T &value) { this->value++; }
  template <class U>
  U finalize()
  {
    return (U)value;
  }
};

template <class T>
class AvgState
{
public:
  AvgState() : value(0), count(0) {}
  T    value;
  int  count = 0;
  void update(const T *values, int size);
  void update(const T &value)
  {
    this->value += value;
    this->count++;
  }
  template <class U>
  U finalize()
  {
    return (U)((float)value / (float)count);
  }
};

void *create_aggregate_state(AggregateExpr::Type aggr_type, AttrType attr_type);

RC aggregate_state_update_by_value(void *state, AggregateExpr::Type aggr_type, AttrType attr_type, const Value &val);
RC aggregate_state_update_by_column(void *state, AggregateExpr::Type aggr_type, AttrType attr_type, Column &col);

RC finialize_aggregate_state(void *state, AggregateExpr::Type aggr_type, AttrType attr_type, Column &col);