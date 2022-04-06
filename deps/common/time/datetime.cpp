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

#include "common/time/datetime.h"

#include "pthread.h"
#include "stdio.h"
#include "string.h"
#include <iomanip>
#include <sstream>
#include <string>
namespace common {

DateTime::DateTime(std::string &xml_str)
{
  tm tmp;
  sscanf(xml_str.c_str(),
      "%04d-%02d-%02dT%02d:%02d:%02dZ",
      &tmp.tm_year,
      &tmp.tm_mon,
      &tmp.tm_mday,
      &tmp.tm_hour,
      &tmp.tm_min,
      &tmp.tm_sec);
  m_date = julian_date(tmp.tm_year, tmp.tm_mon, tmp.tm_mday);
  m_time = make_hms(tmp.tm_hour, tmp.tm_min, tmp.tm_sec, 0);
}

time_t DateTime::str_to_time_t(std::string &xml_str)
{
  tm tmp;
  sscanf(xml_str.c_str(),
      "%04d-%02d-%02dT%02d:%02d:%02dZ",
      &tmp.tm_year,
      &tmp.tm_mon,
      &tmp.tm_mday,
      &tmp.tm_hour,
      &tmp.tm_min,
      &tmp.tm_sec);
  m_date = julian_date(tmp.tm_year, tmp.tm_mon, tmp.tm_mday);
  m_time = make_hms(tmp.tm_hour, tmp.tm_min, tmp.tm_sec, 0);
  return to_time_t();
}

std::string DateTime::time_t_to_str(int timet)
{
  std::ostringstream oss;
  oss << std::dec << std::setw(10) << timet;
  return oss.str();
}

std::string DateTime::time_t_to_xml_str(time_t timet)
{
  std::string ret_val;
  std::ostringstream oss;
  struct tm tmbuf;
  tm *tm_info = gmtime_r(&timet, &tmbuf);
  oss << tm_info->tm_year + 1900 << "-";
  if ((tm_info->tm_mon + 1) <= 9)
    oss << "0";
  oss << tm_info->tm_mon + 1 << "-";
  if (tm_info->tm_mday <= 9)
    oss << "0";
  oss << tm_info->tm_mday << "T";
  if (tm_info->tm_hour <= 9)
    oss << "0";
  oss << tm_info->tm_hour << ":";
  if (tm_info->tm_min <= 9)
    oss << "0";
  oss << tm_info->tm_min << ":";
  if (tm_info->tm_sec <= 9)
    oss << "0";
  oss << tm_info->tm_sec << "Z";
  ret_val = oss.str();
  return ret_val;
}

std::string DateTime::str_to_time_t_str(std::string &xml_str)
{
  tm tmp;
  std::ostringstream oss;
  sscanf(xml_str.c_str(),
      "%04d-%02d-%02dT%02d:%02d:%02dZ",
      &tmp.tm_year,
      &tmp.tm_mon,
      &tmp.tm_mday,
      &tmp.tm_hour,
      &tmp.tm_min,
      &tmp.tm_sec);
  m_date = julian_date(tmp.tm_year, tmp.tm_mon, tmp.tm_mday);
  m_time = make_hms(tmp.tm_hour, tmp.tm_min, tmp.tm_sec, 0);
  time_t timestamp = to_time_t();
  oss << std::dec << std::setw(10) << timestamp;
  return oss.str();
}

time_t DateTime::nowtimet()
{
  struct timeval tv;
  gettimeofday(&tv, 0);
  return tv.tv_sec;
  ;
}

DateTime DateTime::now()
{
  struct timeval tv;
  gettimeofday(&tv, 0);
  return from_time_t(tv.tv_sec, tv.tv_usec / 1000);
}

//! Return date and time as a string in Xml Schema date-time format
std::string DateTime::to_xml_date_time()
{

  std::string ret_val;
  tm tm_info;
  std::ostringstream oss;

  tm_info = to_tm();
  oss << tm_info.tm_year + 1900 << "-";
  if ((tm_info.tm_mon + 1) <= 9)
    oss << "0";
  oss << tm_info.tm_mon + 1 << "-";
  if (tm_info.tm_mday <= 9)
    oss << "0";
  oss << tm_info.tm_mday << "T";
  if (tm_info.tm_hour <= 9)
    oss << "0";
  oss << tm_info.tm_hour << ":";
  if (tm_info.tm_min <= 9)
    oss << "0";
  oss << tm_info.tm_min << ":";
  if (tm_info.tm_sec <= 9)
    oss << "0";
  oss << tm_info.tm_sec << "Z";
  ret_val = oss.str();
  return ret_val;
}

time_t DateTime::add_duration(std::string xml_duration)
{
  add_duration_date_time(xml_duration);
  return to_time_t();
}

void DateTime::add_duration_date_time(std::string xml_duration)
{
  // start datetime values
  int s_year, s_month, s_day;
  int s_hour, s_min, s_sec, s_millis = 0;
  get_ymd(s_year, s_month, s_day);
  get_hms(s_hour, s_min, s_sec, s_millis);

  // temp values
  int tmp_month, tmp_sec, tmp_min, tmp_hour, tmp_day;

  // duration values
  struct tm dur_t;
  parse_duration(xml_duration, dur_t);

  // end values
  int e_year, e_month, e_day, e_hour, e_min, e_sec, e_millis = 0;

  // months
  tmp_month = s_month + dur_t.tm_mon;
  e_month = ((tmp_month - 1) % 12) + 1;
  int carry_month = ((tmp_month - 1) / 12);

  // years
  e_year = s_year + dur_t.tm_year + carry_month;

  // seconds
  tmp_sec = s_sec + dur_t.tm_sec;
  e_sec = tmp_sec % 60;
  int carry_sec = tmp_sec / 60;

  // minutes
  tmp_min = s_min + dur_t.tm_min + carry_sec;
  e_min = tmp_min % 60;
  int carry_min = tmp_min / 60;

  // hours
  tmp_hour = s_hour + dur_t.tm_hour + carry_min;
  e_hour = tmp_hour % 24;
  int carry_hr = tmp_hour / 24;

  // days
  if (s_day > max_day_in_month_for(e_year, e_month)) {
    tmp_day = max_day_in_month_for(e_year, e_month);
  } else {
    if (s_day < 1) {
      tmp_day = 1;
    } else {
      tmp_day = s_day;
    }
  }
  e_day = tmp_day + dur_t.tm_mday + carry_hr;
  int carry_day = 0;
  while (true) {
    if (e_day < 1) {
      e_day = e_day + max_day_in_month_for(e_year, e_month - 1);
      carry_day = -1;
    } else {
      if (e_day > max_day_in_month_for(e_year, e_month)) {
        e_day -= max_day_in_month_for(e_year, e_month);
        carry_day = 1;
      } else {
        break;
      }
    }
    tmp_month = e_month + carry_day;
    e_month = ((tmp_month - 1) % 12) + 1;
    e_year = e_year + (tmp_month - 1) / 12;
  }
  m_date = julian_date(e_year, e_month, e_day);
  m_time = make_hms(e_hour, e_min, e_sec, e_millis);
  return;
}

int DateTime::max_day_in_month_for(int yr, int month)
{
  int tmp_month = ((month - 1) % 12) + 1;
  int tmp_year = yr + ((tmp_month - 1) / 12);

  if (tmp_month == MON_JAN || tmp_month == MON_MAR || tmp_month == MON_MAY || tmp_month == MON_JUL ||
      tmp_month == MON_AUG || tmp_month == MON_OCT || tmp_month == MON_DEC) {
    return 31;
  } else {
    if (tmp_month == MON_APR || tmp_month == MON_JUN || tmp_month == MON_SEP || tmp_month == MON_NOV)
      return 30;
    else {
      if (tmp_month == MON_FEB && ((0 == tmp_year % 400) || ((0 != tmp_year % 100) && 0 == tmp_year % 4))) {
        return 29;
      } else
        return 28;
    }
  }
}

void DateTime::parse_duration(std::string dur_str, struct tm &tm_t)
{
  std::string::size_type index = 0;
  bzero(&tm_t, sizeof(tm_t));
  if (dur_str[index] != 'P') {
    return;
  }
  int ind_t = dur_str.find('T', 0);

  index++;
  int ind_y = dur_str.find('Y', index);
  if (ind_y != -1) {
    int sign = 1;
    if (dur_str[index] == '-') {
      sign = -1;
      index++;
    }
    std::string sY = dur_str.substr(index, ind_y);
    sscanf(sY.c_str(), "%d", &tm_t.tm_year);
    tm_t.tm_year *= sign;
    index = ind_y + 1;
  }

  int ind_m = dur_str.find('M', index);
  if ((ind_m != -1) && (((ind_t > -1) && (ind_m < ind_t)) || ind_t == -1)) {
    int sign = 1;
    if (dur_str[index] == '-') {
      sign = -1;
      index++;
    }
    sscanf(dur_str.substr(index, ind_m).c_str(), "%d", &tm_t.tm_mon);
    tm_t.tm_mon *= sign;
    index = ind_m + 1;
  }

  int ind_d = dur_str.find('D', index);
  int sign = 1;
  if (ind_d != -1) {
    if (dur_str[index] == '-') {
      sign = -1;
      index++;
    }
    sscanf(dur_str.substr(index, ind_d).c_str(), "%d", &tm_t.tm_mday);
    tm_t.tm_mday *= sign;
    // index = ind_d + 1; // not used
  }

  if (ind_t == -1) {
    tm_t.tm_hour = tm_t.tm_min = tm_t.tm_sec = 0;
    return;
  }
  index = ind_t + 1;

  int ind_h = dur_str.find('H', index);
  if (ind_h != -1) {
    int sign = 1;
    if (dur_str[index] == '-') {
      sign = -1;
      index++;
    }
    sscanf(dur_str.substr(index, ind_h).c_str(), "%d", &tm_t.tm_hour);
    tm_t.tm_hour *= sign;
    index = ind_h + 1;
  }

  int ind_min = dur_str.find('M', index);
  if (ind_min != -1) {
    int sign = 1;
    if (dur_str[index] == '-') {
      sign = -1;
      index++;
    }
    sscanf(dur_str.substr(index, ind_min).c_str(), "%d", &tm_t.tm_min);
    tm_t.tm_min *= sign;
    index = ind_min + 1;
  }

  int ind_s = dur_str.find('S', index);
  if (ind_s != -1) {
    int sign = 1;
    if (dur_str[index] == '-') {
      sign = -1;
      index++;
    }
    sscanf(dur_str.substr(index, ind_s).c_str(), "%d", &tm_t.tm_sec);
    tm_t.tm_sec *= sign;
  }
  return;
}

// generate OBJ_ID_TIMESTMP_DIGITS types unique timestamp string
// caller doesn't need get any lock
#define OBJ_ID_TIMESTMP_DIGITS 14
std::string Now::unique()
{
  struct timeval tv;
  u64_t temp;
  static u64_t last_unique = 0;
#if defined(LINUX)
  static pthread_mutex_t mutex = PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP;
#elif defined(__MACH__)
  static pthread_mutex_t mutex = PTHREAD_ERRORCHECK_MUTEX_INITIALIZER;
#endif
  gettimeofday(&tv, NULL);
  temp = (((u64_t)tv.tv_sec) << 20) + tv.tv_usec;
  pthread_mutex_lock(&mutex);
  if (temp > last_unique) {
    // record last timeStamp
    last_unique = temp;
  } else {
    // increase the last timeStamp and use it as unique timestamp
    // as NTP may sync local time clock backward.
    // after time catch up, will change back to use time again.
    last_unique++;
    // set the new last_unique as unique timestamp
    temp = last_unique;
  }
  pthread_mutex_unlock(&mutex);

  // further refine below code, the common time unique function
  //      should not cover OBJ_ID_TIMESTMP_DIGITS, which is only
  //      related with the object id.
  std::ostringstream oss;
  oss << std::hex << std::setw(OBJ_ID_TIMESTMP_DIGITS) << std::setfill('0') << temp;
  return oss.str();
}

bool DateTime::is_valid_xml_datetime(const std::string &str)
{
  // check length. 20 is the length of a xml date
  if (str.length() != 20)
    return false;

  // check each character is correct
  const char *const flag = "0000-00-00T00:00:00Z";
  for (unsigned int i = 0; i < str.length(); ++i) {
    if (flag[i] == '0') {
      if (!isdigit(str[i]))
        return false;
    } else if (flag[i] != str[i]) {
      return false;
    }
  }

  // check month, date, hour, min, second is valid
  tm tmp;
  int ret = sscanf(str.c_str(),
      "%04d-%02d-%02dT%02d:%02d:%02dZ",
      &tmp.tm_year,
      &tmp.tm_mon,
      &tmp.tm_mday,
      &tmp.tm_hour,
      &tmp.tm_min,
      &tmp.tm_sec);

  if (ret != 6)
    return false;  // should have 6 match

  if (tmp.tm_mon > 12 || tmp.tm_mon <= 0)
    return false;
  if (tmp.tm_mday > 31 || tmp.tm_mday <= 0)
    return false;
  if (tmp.tm_hour > 24 || tmp.tm_hour < 0)
    return false;
  if (tmp.tm_min > 60 || tmp.tm_min < 0)
    return false;
  if (tmp.tm_sec > 60 || tmp.tm_sec < 0)
    return false;

  return true;
}

}  // namespace common