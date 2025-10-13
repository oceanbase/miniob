/*
 * date_type.cpp - DATE 类型支持实现
 */
#include "common/type/date_type.h"
#include "common/value.h"
#include <sstream>
#include <iomanip>

// 判断闰年
static bool is_leap(int year) {
  return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

// 校验日期是否合法
bool DateType::is_valid_date(int year, int month, int day) {
  if (year < 0 || month < 1 || month > 12 || day < 1) return false;
  static const int days[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
  int mdays = days[month-1];
  if (month == 2 && is_leap(year)) mdays = 29;
  return day <= mdays;
}

int DateType::compare(const Value &left, const Value &right) const {
  int ly, lm, ld, ry, rm, rd;
  left.get_date(ly, lm, ld);
  right.get_date(ry, rm, rd);
  if (ly != ry) return ly < ry ? -1 : 1;
  if (lm != rm) return lm < rm ? -1 : 1;
  if (ld != rd) return ld < rd ? -1 : 1;
  return 0;
}

RC DateType::cast_to(const Value &val, AttrType type, Value &result) const {
  if (type == AttrType::DATES) {
    result = val;
    return RC::SUCCESS;
  }
  return RC::UNSUPPORTED;
}

RC DateType::to_string(const Value &val, std::string &str) const {
  int y, m, d;
  val.get_date(y, m, d);
  std::ostringstream oss;
  oss << std::setw(4) << std::setfill('0') << y << "-"
      << std::setw(2) << std::setfill('0') << m << "-"
      << std::setw(2) << std::setfill('0') << d;
  str = oss.str();
  return RC::SUCCESS;
}

RC DateType::set_value_from_str(Value &val, const std::string &data) const {
  int y, m, d;
  if (sscanf(data.c_str(), "%d-%d-%d", &y, &m, &d) == 3 && is_valid_date(y, m, d)) {
    val.set_date(y, m, d);
    return RC::SUCCESS;
  }
  return RC::INVALID_ARGUMENT;
}
