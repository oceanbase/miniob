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
#include <string>

/**
 * @brief 日期类型
 * @ingroup DataType
 */
class DateType : public DataType
{
public:
  DateType() : DataType(AttrType::DATES) {}
  virtual ~DateType() = default;

  int compare(const Value &left, const Value &right) const override;

  RC set_value_from_str(Value &val, const string &data) const override;

  RC to_string(const Value &val, string &result) const override;
  
  RC cast_to(const Value &val, AttrType type, Value &result) const override;
  
  int cast_cost(AttrType type) override;
  
  // 日期内部表示为YYYYMMDD的整数格式
  // 检查日期有效性，包括闰年
  static bool is_valid_date(int year, int month, int day);
  
  // 将字符串日期转换为内部表示
  static int date_str_to_int(const string &date_str);
  
  // 将内部表示转换为字符串日期
  static string date_int_to_str(int date_int);
};