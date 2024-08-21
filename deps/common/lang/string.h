/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
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

#pragma once

// Basic includes
#include <cxxabi.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include <cstdlib>
#include <string>
#include <typeinfo>

#include "common/defs.h"
#include "common/lang/vector.h"
#include "common/lang/iostream.h"
#include "common/lang/sstream.h"
#include "common/lang/set.h"

using std::string;

namespace common {

/**
 * remove all white space(like ' ', '\t', '\n') from string
 */
void  strip(string &str);
char *strip(char *str);

/**
 * Convert an integer size in a padded string
 * @param[in]   size    size to be converted and 0 padded
 * @param[in]   pad     decimal digits for the string padding
 * return   output 0-padded size string
 */
string size_to_pad_str(int size, int pad);

/**
 * Convert a string to upper case.
 * @param[in,out] s the string to modify
 * @return a reference to the string that was modified.
 */
string &str_to_upper(string &s);

/**
 * Convert a string to lower case.
 * @param[in,out] s the string to modify
 * @return a reference to the string that was modified.
 */
string &str_to_lower(string &s);

/**
 * Split string str using 'delimiter's
 * @param[in]      str        the string to be split up
 * @param[in]      delims     elimiter characters
 * @param[in,out] results     ector containing the split up string
 */
void split_string(const string &str, string delim, set<string> &results);
void split_string(const string &str, string delim, vector<string> &results);
void split_string(char *str, char dim, vector<char *> &results, bool keep_null = false);

void merge_string(string &str, string delim, vector<string> &result, size_t result_len = 0);
/**
 * replace old with new in the string
 */
void replace(string &str, const string &old, const string &new_str);

/**
 * binary to hexadecimal
 */
char *bin_to_hex(const char *s, const int len, char *hex_buff);
/**
 * hexadecimal to binary
 */
char *hex_to_bin(const char *s, char *bin_buff, int *dest_len);

/**
 * Convert a number in a string format to a numeric value
 * @param[in]   str     input number string
 * @param[out]  val     output value
 * @param[in]   radix   an optional parameter that specifies the
 *                      radix for the conversion.  By default, the
 *                      radix is set to std::dec (decimal).  See
 *                      also, std::oct (octal) and std::hex
 *                      (hexidecimal).
 * @return \c true if the string was successfully converted to a
 *         number, \c false otherwise
 */
template <class T>
bool str_to_val(const string &str, T &val, ios_base &(*radix)(ios_base &) = std::dec);

/**
 * Convert a numeric value into its string representation
 * @param[in]   val     numeric value
 * @param[out]  str     string representation of the numeric value
 * @param[in]   radix   an optional parameter that specifies the
 *                      radix for the conversion.  By default, the
 *                      radix is set to std::dec (decimal).  See
 *                      also, std::oct (octal) and std::hex
 *                      (hexidecimal).
 */
template <class T>
void val_to_str(const T &val, string &str, ios_base &(*radix)(ios_base &) = std::dec);

/**
 * Double to string
 * @param v
 * @return
 */
string double_to_str(double v);

bool is_blank(const char *s);

/**
 * 获取子串
 * 从s中提取下标为n1~n2的字符组成一个新字符串，然后返回这个新串的首地址
 *
 * @param s
 * @param n1
 * @param n2
 * @return
 */
char *substr(const char *s, int n1, int n2);

/**
 * get type's name
 */
template <class T>
string get_type_name(const T &val);

template <class T>
bool str_to_val(const string &str, T &val, ios_base &(*radix)(ios_base &)/* = std::dec */)
{
  bool          success = true;
  istringstream is(str);
  if (!(is >> radix >> val)) {
    val     = 0;
    success = false;
  }
  return success;
}

template <class T>
void val_to_str(const T &val, string &str, ios_base &(*radix)(ios_base &)/* = std::dec */)
{
  stringstream strm;
  strm << radix << val;
  str = strm.str();
}

template <class T>
string get_type_name(const T &val)
{
  int   status = 0;
  char *stmp   = abi::__cxa_demangle(typeid(val).name(), 0, 0, &status);
  if (!stmp)
    return "";

  string sret(stmp);

  ::free(stmp);
  return sret;
}

}  // namespace common
