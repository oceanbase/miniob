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

#include <cstdio>

#include "common/lang/comparator.h"
#include "common/lang/sstream.h"
#include "common/log/log.h"
#include "common/value.h"

int DateType::compare(const Value &left, const Value &right) const
{
  ASSERT(left.attr_type() == AttrType::DATES, "left type is not date");
  ASSERT(right.attr_type() == AttrType::DATES, "right type is not date");
  return common::compare_int((void *)&left.value_.int_value_, (void *)&right.value_.int_value_);
}

RC DateType::cast_to(const Value &val, AttrType type, Value &result) const
{
  switch (type) {
    case AttrType::DATES: {
      result.set_type(AttrType::DATES);
      result.set_data(val.data(), static_cast<int>(sizeof(int32_t)));
      return RC::SUCCESS;
    }
    case AttrType::CHARS:
    case AttrType::TEXTS: {
      string date_string;
      RC     rc = to_string(val, date_string);
      if (OB_FAIL(rc)) {
        return rc;
      }
      result.set_string(date_string.c_str(), static_cast<int>(date_string.size()), type);
      return RC::SUCCESS;
    }
    default: return RC::SCHEMA_FIELD_TYPE_MISMATCH;
  }
}

int DateType::cast_cost(AttrType type)
{
  if (type == AttrType::DATES) {
    return 0;
  }
  if (type == AttrType::CHARS || type == AttrType::TEXTS) {
    return 2;
  }
  return INT32_MAX;
}

RC DateType::set_value_from_str(Value &val, const string &data) const
{
  int32_t encoded_date = 0;
  RC      rc           = parse_date_string(data, encoded_date);
  if (OB_SUCC(rc)) {
    val.set_type(AttrType::DATES);
    val.set_data(reinterpret_cast<char *>(&encoded_date), static_cast<int>(sizeof(encoded_date)));
  }
  return rc;
}

RC DateType::to_string(const Value &val, string &result) const
{
  ASSERT(val.attr_type() == AttrType::DATES, "value type is not date");

  const int32_t encoded_date = val.value_.int_value_;
  const int     year         = encoded_date / 10000;
  const int     month        = (encoded_date / 100) % 100;
  const int     day          = encoded_date % 100;

  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d", year, month, day);
  result = buffer;
  return RC::SUCCESS;
}

RC DateType::parse_date_string(const string &data, int32_t &encoded_date)
{
  int  year  = 0;
  int  month = 0;
  int  day   = 0;
  char dash1 = 0;
  char dash2 = 0;

  stringstream deserialize_stream;
  deserialize_stream.clear();
  deserialize_stream.str(data);
  deserialize_stream >> year >> dash1 >> month >> dash2 >> day;
  if (!deserialize_stream || !deserialize_stream.eof() || dash1 != '-' || dash2 != '-') {
    return RC::SCHEMA_FIELD_TYPE_MISMATCH;
  }

  if (!is_valid_date(year, month, day)) {
    return RC::SCHEMA_FIELD_TYPE_MISMATCH;
  }

  encoded_date = year * 10000 + month * 100 + day;
  return RC::SUCCESS;
}

bool DateType::is_leap_year(int year) { return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0); }

bool DateType::is_valid_date(int year, int month, int day)
{
  static constexpr int days_per_month[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

  if (month < 1 || month > 12 || day < 1) {
    return false;
  }

  int max_day = days_per_month[month];
  if (month == 2 && is_leap_year(year)) {
    max_day = 29;
  }
  return day <= max_day;
}
