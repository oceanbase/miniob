/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Longda on 2010
//

#ifndef __COMMON_TIME_DATETIME_H__
#define __COMMON_TIME_DATETIME_H__

#include <math.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>

#include <iomanip>
#include <iostream>

#include "common/defs.h"

namespace common {

/*
 *  \brief Date and time are represented as integer for ease of
 *   calculation and comparison.
 *
 *  Julian day number is the integer number of days that have elapsed since
 *  the defined as noon Universal Time (UT) Monday, January 1, 4713 BC.
 *
 *  Date and Time stored as a Julian day number and number of
 *  milliseconds since midnight.  Does not perform any timezone
 *  calculations.  All magic numbers and related calculations
 *  have been taken from:
 *
 *  \sa http://www.faqs.org/faqs/calendars.faq
 *  \sa http://scienceworld.wolfram.com/astronomy/JulianDate.html
 *  \sa http://scienceworld.wolfram.com/astronomy/GregorianCalendar.html
 *  \sa http://scienceworld.wolfram.com/astronomy/Weekday.html
 */

struct DateTime {
  int m_date;
  int m_time;

  enum {
    SECONDS_PER_DAY = 86400,
    SECONDS_PER_HOUR = 3600,
    SECONDS_PER_MIN = 60,
    MINUTES_PER_HOUR = 60,

    MILLIS_PER_DAY = 86400000,
    MILLIS_PER_HOUR = 3600000,
    MILLIS_PER_MIN = 60000,
    MILLIS_PER_SEC = 1000,

    // time_t epoch (1970-01-01) as a Julian date
    JULIAN_19700101 = 2440588
  };
  enum {
    MON_JAN = 1,
    MON_FEB = 2,
    MON_MAR = 3,
    MON_APR = 4,
    MON_MAY = 5,
    MON_JUN = 6,
    MON_JUL = 7,
    MON_AUG = 8,
    MON_SEP = 9,
    MON_OCT = 10,
    MON_NOV = 11,
    MON_DEC = 12
  };

  // Default constructor - initializes to zero
  DateTime() : m_date(0), m_time(0)
  {}

  // Construct from a Julian day number and time in millis
  DateTime(int date, int time) : m_date(date), m_time(time)
  {}

  // Construct from the specified components
  DateTime(int year, int month, int day, int hour, int minute, int second, int millis)
  {
    m_date = julian_date(year, month, day);
    m_time = make_hms(hour, minute, second, millis);
  }

  // Construct from the xml datetime format
  DateTime(std::string &xml_time);

  // check whether a string is valid with a xml datetime format
  static bool is_valid_xml_datetime(const std::string &str);

  // Load the referenced values with the year, month and day
  // portions of the date in a single operation
  inline void get_ymd(int &year, int &month, int &day) const
  {
    get_ymd(m_date, year, month, day);
  }

  // Load the referenced values with the hour, minute, second and
  // millisecond portions of the time in a single operation
  inline void get_hms(int &hour, int &minute, int &second, int &millis) const
  {
    int ticks = m_time / MILLIS_PER_SEC;
    hour = ticks / SECONDS_PER_HOUR;
    minute = (ticks / SECONDS_PER_MIN) % MINUTES_PER_HOUR;
    second = ticks % SECONDS_PER_MIN;
    millis = m_time % MILLIS_PER_SEC;
  }

  // Convert the DateTime to a time_t.  Note that this operation
  // can overflow on 32-bit platforms when we go beyond year 2038.
  inline time_t to_time_t() const
  {
    return (SECONDS_PER_DAY * (m_date - JULIAN_19700101) + m_time / MILLIS_PER_SEC);
  }

  // Convert the DateTime to a struct tm which is in UTC
  tm to_tm() const
  {
    int year, month, day;
    int hour, minute, second, millis;
    tm result = {0};

    get_ymd(year, month, day);
    get_hms(hour, minute, second, millis);

    result.tm_year = year - 1900;
    result.tm_mon = month - 1;
    result.tm_mday = day;
    result.tm_hour = hour;
    result.tm_min = minute;
    result.tm_sec = second;
    result.tm_isdst = -1;

    return result;
  }

  // Set the date portion of the DateTime
  void set_ymd(int year, int month, int day)
  {
    m_date = julian_date(year, month, day);
  }

  // Set the time portion of the DateTime
  void set_hms(int hour, int minute, int second, int millis)
  {
    m_time = make_hms(hour, minute, second, millis);
  }

  // Clear the date portion of the DateTime
  void clear_date()
  {
    m_date = 0;
  }

  // Clear the time portion of the DateTime
  void clear_time()
  {
    m_time = 0;
  }

  // Set the internal date and time members
  void set(int date, int time)
  {
    m_date = date;
    m_time = time;
  }

  // Initialize from another DateTime
  void set(const DateTime &other)
  {
    m_date = other.m_date;
    m_time = other.m_time;
  }

  // Add a number of seconds to this
  void operator+=(int seconds)
  {
    int d = seconds / SECONDS_PER_DAY;
    int s = seconds % SECONDS_PER_DAY;

    m_date += d;
    m_time += s * MILLIS_PER_SEC;

    if (m_time > MILLIS_PER_DAY) {
      m_date++;
      m_time %= MILLIS_PER_DAY;
    } else if (m_time < 0) {
      m_date--;
      m_time += MILLIS_PER_DAY;
    }
  }

  // Return date and time as a string in XML Schema Date-Time format
  std::string to_xml_date_time();

  // Return time_t from XML schema date-time format.
  time_t str_to_time_t(std::string &xml_str);

  // Return xml time str from time_t.
  std::string time_t_to_xml_str(time_t timet);

  // Return time_t str from time_t.
  std::string time_t_to_str(int timet);

  // Return time_t string from XML schema date-time format.
  std::string str_to_time_t_str(std::string &xml_str);

  // Helper method to convert a broken down time to a number of
  // milliseconds since midnight
  static int make_hms(int hour, int minute, int second, int millis)
  {
    return MILLIS_PER_SEC * (SECONDS_PER_HOUR * hour + SECONDS_PER_MIN * minute + second) + millis;
  }

  // Return the current wall-clock time as a DateTime
  static DateTime now();

  // Return the current wall-clock time as time_t
  time_t nowtimet();

  // Convert a time_t and optional milliseconds to a DateTime
  static DateTime from_time_t(time_t t, int millis = 0)
  {
    struct tm tmbuf;
    tm *tm = gmtime_r(&t, &tmbuf);
    return from_tm(*tm, millis);
  }

  // Convert a tm and optional milliseconds to a DateTime.  \note
  // the tm structure is assumed to contain a date specified in UTC
  static DateTime from_tm(const tm &tm, int millis = 0)
  {
    return DateTime(
        julian_date(tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday), make_hms(tm.tm_hour, tm.tm_min, tm.tm_sec, millis));
  }

  // Helper method to calculate a Julian day number.
  static int julian_date(int year, int month, int day)
  {
    int a = (14 - month) / 12;
    int y = year + 4800 - a;
    int m = month + 12 * a - 3;
    return (day + int((153 * m + 2) / 5) + y * 365 + int(y / 4) - int(y / 100) + int(y / 400) - 32045);
  }

  // Convert a Julian day number to a year, month and day
  static void get_ymd(int jday, int &year, int &month, int &day)
  {
    int a = jday + 32044;
    int b = (4 * a + 3) / 146097;
    int c = a - int((b * 146097) / 4);
    int d = (4 * c + 3) / 1461;
    int e = c - int((1461 * d) / 4);
    int m = (5 * e + 2) / 153;
    day = e - int((153 * m + 2) / 5) + 1;
    month = m + 3 - 12 * int(m / 10);
    year = b * 100 + d - 4800 + int(m / 10);
  }

  // Return a human-friendly string representation of the timestamp,
  // expressed in terms of the local timezone
  std::string to_string_local()
  {
    const time_t tt = to_time_t();
    // 'man asctime' specifies that buffer must be at least 26 bytes
    char buffer[32];
    struct tm tm;
    asctime_r(localtime_r(&tt, &tm), &(buffer[0]));
    std::string s(buffer);
    return s;
  }

  // Return a human-friendly string representation of the timestamp,
  // expressed in terms of Coordinated Universal Time (UTC)
  std::string to_string_utc()
  {
    const time_t tt = to_time_t();
    // 'man asctime' specifies that buffer must be at least 26 bytes
    char buffer[32];
    struct tm tm;
    asctime_r(gmtime_r(&tt, &tm), &(buffer[0]));
    std::string s(buffer);
    return s;
  }

  // add duration to this time
  time_t add_duration(std::string xml_dur);

  // add duration to this time
  void add_duration_date_time(std::string xml_dur);

  // add duration to this time
  int max_day_in_month_for(int year, int month);

  // parse the duration string and convert it to struct tm
  void parse_duration(std::string dur_str, struct tm &tm_t);
};

inline bool operator==(const DateTime &lhs, const DateTime &rhs)
{
  return lhs.m_date == rhs.m_date && lhs.m_time == rhs.m_time;
}

inline bool operator!=(const DateTime &lhs, const DateTime &rhs)
{
  return !(lhs == rhs);
}

inline bool operator<(const DateTime &lhs, const DateTime &rhs)
{
  if (lhs.m_date < rhs.m_date)
    return true;
  else if (lhs.m_date > rhs.m_date)
    return false;
  else if (lhs.m_time < rhs.m_time)
    return true;
  return false;
}

inline bool operator>(const DateTime &lhs, const DateTime &rhs)
{
  return !(lhs == rhs || lhs < rhs);
}

inline bool operator<=(const DateTime &lhs, const DateTime &rhs)
{
  return lhs == rhs || lhs < rhs;
}

inline bool operator>=(const DateTime &lhs, const DateTime &rhs)
{
  return lhs == rhs || lhs > rhs;
}

// Calculate the difference between two DateTime values and return
// the result as a number of seconds
inline int operator-(const DateTime &lhs, const DateTime &rhs)
{
  return (DateTime::SECONDS_PER_DAY * (lhs.m_date - rhs.m_date) +
          // Truncate the millis before subtracting
          lhs.m_time / 1000 - rhs.m_time / 1000);
}

// Date and Time represented in UTC.
class TimeStamp : public DateTime {
public:
  // Defaults to the current date and time
  TimeStamp() : DateTime(DateTime::now())
  {}

  // Defaults to the current date
  TimeStamp(int hour, int minute, int second, int millisecond = 0) : DateTime(DateTime::now())
  {
    set_hms(hour, minute, second, millisecond);
  }

  TimeStamp(int hour, int minute, int second, int date, int month, int year)
      : DateTime(year, month, date, hour, minute, second, 0)
  {}

  TimeStamp(int hour, int minute, int second, int millisecond, int date, int month, int year)
      : DateTime(year, month, date, hour, minute, second, millisecond)
  {}

  TimeStamp(time_t time, int millisecond = 0) : DateTime(from_time_t(time, millisecond))
  {}

  TimeStamp(const tm *time, int millisecond = 0) : DateTime(from_tm(*time, millisecond))
  {}

  void set_current()
  {
    set(DateTime::now());
  }
};

// Time only represented in UTC.
class Time : public DateTime {
public:
  // Defaults to the current time
  Time()
  {
    set_current();
  }

  Time(const DateTime &val) : DateTime(val)
  {
    clear_date();
  }

  Time(int hour, int minute, int second, int millisecond = 0)
  {
    set_hms(hour, minute, second, millisecond);
  }

  Time(time_t time, int millisecond = 0) : DateTime(from_time_t(time, millisecond))
  {
    clear_date();
  }

  Time(const tm *time, int millisecond = 0) : DateTime(from_tm(*time, millisecond))
  {
    clear_date();
  }

  // Set to the current time.
  void set_current()
  {
    DateTime d = now();
    m_time = d.m_time;
  }
};

// Date only represented in UTC.
class Date : public DateTime {
public:
  // Defaults to the current date
  Date()
  {
    set_current();
  }

  Date(const DateTime &val) : DateTime(val)
  {
    clear_time();
  }

  Date(int date, int month, int year) : DateTime(year, month, date, 0, 0, 0, 0)
  {}

  Date(long sec) : DateTime(sec / DateTime::SECONDS_PER_DAY, 0)
  {}

  Date(const tm *time) : DateTime(from_tm(*time))
  {
    clear_time();
  }

  // Set to the current time.
  void set_current()
  {
    DateTime d = now();
    m_date = d.m_date;
  }
};

class Now {
public:
  static inline s64_t sec()
  {
    struct timeval tv;
    gettimeofday(&tv, 0);
    time_t sec = tv.tv_sec;
    // Round up if necessary
    if (tv.tv_usec > 500 * 1000)
      sec++;
    return sec;
  }

  static inline s64_t usec()
  {
    struct timeval tv;
    gettimeofday(&tv, 0);
    return (s64_t)tv.tv_sec * 1000000 + tv.tv_usec;
  }

  static inline s64_t msec()
  {
    struct timeval tv;
    gettimeofday(&tv, 0);
    s64_t msec = (s64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
    if (tv.tv_usec % 1000 >= 500)
      msec++;
    return msec;
  }

  static std::string unique();
};

}  // namespace common
#endif  //__COMMON_TIME_DATETIME_H__
