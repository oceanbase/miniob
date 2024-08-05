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

#include "common/lang/array.h"
#include "common/lang/memory.h"
#include "common/lang/string.h"
#include "common/rc.h"
#include "common/type/attr_type.h"

class Value;

class DataType
{
public:
  explicit DataType(AttrType attr_type) : attr_type_(attr_type) {}

  virtual ~DataType() = default;

  inline static DataType *type_instance(AttrType attr_type)
  {
    return type_instances_.at(static_cast<int>(attr_type)).get();
  }

  inline AttrType get_attr_type() const { return attr_type_; }

  /**
   * @return
   *  -1 表示 left < right
   *  0 表示 left = right
   *  1 表示 left > right
   *  INT32_MAX 表示未实现的比较
   */
  virtual int compare(const Value &left, const Value &right) const { return INT32_MAX; }

  virtual RC add(const Value &left, const Value &right, Value &result) const { return RC::UNSUPPORTED; }
  virtual RC subtract(const Value &left, const Value &right, Value &result) const { return RC::UNSUPPORTED; }
  virtual RC multiply(const Value &left, const Value &right, Value &result) const { return RC::UNSUPPORTED; }
  virtual RC divide(const Value &left, const Value &right, Value &result) const { return RC::UNSUPPORTED; }
  virtual RC negative(const Value &val, Value &result) const { return RC::UNSUPPORTED; }

  virtual RC set_value_from_str(Value &val, const string &data) const { return RC::UNSUPPORTED; }

protected:
  AttrType attr_type_;

  static array<unique_ptr<DataType>, static_cast<int>(AttrType::MAXTYPE)> type_instances_;
};