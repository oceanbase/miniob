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

#include "common/type/data_type.h"

/**
 * @brief 浮点型数据类型
 * @ingroup DataType
 */
class FloatType : public DataType
{
public:
  FloatType() : DataType(AttrType::FLOATS) {}
  virtual ~FloatType() = default;

  int compare(const Value &left, const Value &right) const override;

  RC add(const Value &left, const Value &right, Value &result) const override;
  RC max(const Value &left, const Value &right, Value &result) const override;
  RC min(const Value &left, const Value &right, Value &result) const override;
  RC avg(const Value &left, const int num, const Value &right, Value &result) const override;
  RC subtract(const Value &left, const Value &right, Value &result) const override;
  RC multiply(const Value &left, const Value &right, Value &result) const override;
  RC divide(const Value &left, const Value &right, Value &result) const override;
  RC negative(const Value &val, Value &result) const override;
  RC set_value_from_str(Value &val, const string &data) const override;
  RC to_string(const Value &val, string &result) const override;

  int cast_cost(AttrType type) override;  // 浮点 => 其他类型的转换衡量
  RC cast_to(const Value &val, AttrType type, Value &result) const override; // 将 val 转换为 type 类型，并将结果保存到 result 中
};