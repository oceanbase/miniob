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
#ifndef DATE_H_
#define DATE_H_

#include <string>
#include <cstdint>
#include <array>
#include <iomanip>  // for std::setw, std::setfill
#include <sstream>  // for std::ostringstream
#include <string>

struct Date {
    uint16_t year;
    uint8_t  month;
    uint8_t  day;

    inline bool isValid() const;

    static Date from_string(std::string const &date_str);

    inline std::string to_string() const;

  private:
    static constexpr std::array<int, 12> daysInMonth = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    /* In this case, 1970 <= year <= 2038 */
    inline bool isValidYear() const;
    inline bool isValidMonth() const;
    inline bool isLeapYear() const;
    inline bool isValidDay() const;
};

inline int compare_date(const Date &d1, const Date &d2)
{
    // Compare year first
    if (d1.year != d2.year) {
        return (d1.year < d2.year) ? -1 : 1;
    }

    // Compare month next
    if (d1.month != d2.month) {
        return (d1.month < d2.month) ? -1 : 1;
    }

    // Finally compare day
    if (d1.day != d2.day) {
        return (d1.day < d2.day) ? -1 : 1;
    }

    // If all are equal, return 0
    return 0;
}

////////////////////
// Implementation //
////////////////////

inline std::string Date::to_string() const {
    std::ostringstream oss;
    oss << year << '-'
        << std::setw(2) << std::setfill('0') << static_cast<int>(month) << '-'
        << std::setw(2) << std::setfill('0') << static_cast<int>(day);
    return oss.str();
}

inline bool Date::isValid() const {
    return isValidYear() && isValidMonth() && isValidDay();
}

/* In this case, 1970 <= year <= 2038 */
inline bool Date::isValidYear() const {
    return year >= 1970 && year <= 2038;
}

inline bool Date::isValidMonth() const {
    return month <= 12 && month >= 1;
}

inline bool Date::isLeapYear() const {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

inline bool Date::isValidDay() const {
    if (!isValidMonth()) return false;

    if (month == 2 && isLeapYear()) {
        return (day >= 1 && day <= 29);
    } else {
        return (day >= 1 && day <= daysInMonth[month - 1]);
    }
}

#endif // DATE_H_
