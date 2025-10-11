/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
        http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "common/type/date_type.h"
#include "common/log/log.h"
#include "common/value.h"
#include "common/lang/string.h"
#include "common/lang/sstream.h"

#include <regex>

// 检查是否是闰年
bool is_leap_year(int year)
{
  if (year % 4 != 0) {
    return false;
  } else if (year % 100 != 0) {
    return true;
  } else if (year % 400 != 0) {
    return false;
  } else {
    return true;
  }
}

// 获取指定月份的天数
int get_days_in_month(int year, int month)
{
  switch (month) {
    case 1: return 31;
    case 2: return is_leap_year(year) ? 29 : 28;
    case 3: return 31;
    case 4: return 30;
    case 5: return 31;
    case 6: return 30;
    case 7: return 31;
    case 8: return 31;
    case 9: return 30;
    case 10: return 31;
    case 11: return 30;
    case 12: return 31;
    default: return 0;
  }
}

// 检查日期有效性
bool DateType::is_valid_date(int year, int month, int day)
{
  // 检查月份范围
  if (month < 1 || month > 12) {
    return false;
  }
  
  // 检查日期范围
  int days_in_month = get_days_in_month(year, month);
  if (day < 1 || day > days_in_month) {
    return false;
  }
  
  return true;
}

// 将字符串日期转换为内部表示
int DateType::date_str_to_int(const string &date_str)
{
  // 日期格式：YYYY-MM-DD
  int year = 0, month = 0, day = 0;
  
  // 使用正则表达式验证日期格式
  std::regex date_pattern("^(\\d{4})-(\\d{2})-(\\d{2})$");
  std::smatch match;
  
  if (!std::regex_match(date_str, match, date_pattern)) {
    LOG_WARN("Invalid date format: %s", date_str.c_str());
    return -1;
  }
  
  try {
    year = std::stoi(match[1]);
    month = std::stoi(match[2]);
    day = std::stoi(match[3]);
  } catch (const std::exception &e) {
    LOG_WARN("Failed to parse date: %s, error: %s", date_str.c_str(), e.what());
    return -1;
  }
  
  // 验证日期有效性
  if (!is_valid_date(year, month, day)) {
    LOG_WARN("Invalid date: %d-%d-%d", year, month, day);
    return -1;
  }
  
  // 转换为YYYYMMDD格式的整数
  return year * 10000 + month * 100 + day;
}

// 将内部表示转换为字符串日期
string DateType::date_int_to_str(int date_int)
{
  if (date_int < 0) {
    return "invalid date";
  }
  
  int year = date_int / 10000;
  int month = (date_int % 10000) / 100;
  int day = date_int % 100;
  
  stringstream ss;
  ss << year << "-" << (month < 10 ? "0" : "") << month << "-" << (day < 10 ? "0" : "") << day;
  return ss.str();
}

// 比较两个日期值
int DateType::compare(const Value &left, const Value &right) const
{
  if (left.attr_type() != AttrType::DATES || right.attr_type() != AttrType::DATES) {
    LOG_WARN("Cannot compare different types: %d vs %d", left.attr_type(), right.attr_type());
    return INT32_MAX;
  }
  
  int left_date = left.get_int();
  int right_date = right.get_int();
  
  if (left_date < right_date) {
    return -1;
  } else if (left_date > right_date) {
    return 1;
  } else {
    return 0;
  }
}

// 从字符串设置日期值
RC DateType::set_value_from_str(Value &val, const string &data) const
{
  RC rc = RC::SUCCESS;
  
  // 去除字符串两端的引号（如果有）
  string date_str = data;
  if (data.front() == '\'' || data.front() == '"') {
    date_str = data.substr(1, data.length() - 2);
  }
  
  int date_int = date_str_to_int(date_str);
  if (date_int < 0) {
    LOG_WARN("Invalid date string: %s", data.c_str());
    rc = RC::SCHEMA_FIELD_TYPE_MISMATCH;
  } else {
    val.set_int(date_int);
    val.set_type(AttrType::DATES);
  }
  
  return rc;
}

// 将日期值转换为字符串
RC DateType::to_string(const Value &val, string &result) const
{
  if (val.attr_type() != AttrType::DATES) {
    LOG_WARN("Cannot convert non-date type to string");
    return RC::INVALID_ARGUMENT;
  }
  
  int date_int = val.get_int();
  result = date_int_to_str(date_int);
  return RC::SUCCESS;
}

// 类型转换
RC DateType::cast_to(const Value &val, AttrType type, Value &result) const
{
  if (val.attr_type() != AttrType::DATES) {
    LOG_WARN("Cannot cast non-date type");
    return RC::INVALID_ARGUMENT;
  }
  
  RC rc = RC::SUCCESS;
  
  switch (type) {
    case AttrType::CHARS: {
      string date_str;
      rc = to_string(val, date_str);
      if (OB_SUCC(rc)) {
        result.set_string(date_str.c_str());
      }
    } break;
    case AttrType::INTS: {
      result.set_int(val.get_int());
    } break;
    case AttrType::DATES: {
      result.set_value(val);
    } break;
    default: {
      LOG_WARN("Unsupported cast type: %d", type);
      rc = RC::UNSUPPORTED;
    } break;
  }
  
  if (OB_SUCC(rc)) {
    result.set_type(type);
  }
  
  return rc;
}

// 计算类型转换成本
int DateType::cast_cost(AttrType type)
{
  if (type == AttrType::DATES) {
    return 0;
  } else if (type == AttrType::CHARS || type == AttrType::INTS) {
    return 1;
  }
  return INT32_MAX;
}