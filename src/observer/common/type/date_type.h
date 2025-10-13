/*
 * date_type.h - DATE 类型支持
 */
#pragma once

// 前向声明 Value
class Value;

#include <string>
#include "common/type/data_type.h"

class DateType : public DataType {
public:
  DateType() : DataType(AttrType::DATES) {}
  ~DateType() override = default;

  int compare(const Value &left, const Value &right) const override;
  RC cast_to(const Value &val, AttrType type, Value &result) const override;
  RC to_string(const Value &val, std::string &str) const override;
  RC set_value_from_str(Value &val, const std::string &data) const override;
  RC add(const Value &left, const Value &right, Value &result) const override { return RC::UNSUPPORTED; }
  RC subtract(const Value &left, const Value &right, Value &result) const override { return RC::UNSUPPORTED; }
  RC multiply(const Value &left, const Value &right, Value &result) const override { return RC::UNSUPPORTED; }
  RC divide(const Value &left, const Value &right, Value &result) const override { return RC::UNSUPPORTED; }
  RC negative(const Value &val, Value &result) const override { return RC::UNSUPPORTED; }

  // 日期合法性校验
  static bool is_valid_date(int year, int month, int day);
};
