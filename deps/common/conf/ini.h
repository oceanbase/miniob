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

#if !defined(__COMMON_CONF_INI_H__)
#define __COMMON_CONF_INI_H__

#include <stdio.h>

#include <iostream>
#include <map>
#include <set>
#include <string>

namespace common {

//********************************************************************
//#means comments
// Ini configuration format
//[section]
// VARNAME=VALUE

class Ini {
public:
  /**
   * To simplify the logic, no lock's when loading configuration
   * So don't modify the data parallel
   */
  Ini();
  ~Ini();

  /**
   * load one ini configuration
   * it support load multiple ini configuration files
   * @return, 0 means success, others means failed
   */
  int load(const std::string &ini_file);

  /**
   * get the map of the section
   * if the section doesn't exist, return one empty section
   */
  const std::map<std::string, std::string> &get(const std::string &section = DEFAULT_SECTION);

  /**
   * get the value of the key in the section,
   * if the key-value doesn't exist,
   * use the input default_value
   */
  std::string get(
      const std::string &key, const std::string &default_value, const std::string &section = DEFAULT_SECTION);

  /**
   * put the key-value pair to the section
   * if the key-value already exist, just replace it
   * if the section doesn't exist, it will create this section
   */
  int put(const std::string &key, const std::string &value, const std::string &section = DEFAULT_SECTION);

  /**
   * output all configuration to one string
   */
  void to_string(std::string &output_str);

  static const std::string DEFAULT_SECTION;

  // one line max length
  static const int MAX_CFG_LINE_LEN = 1024;

  // value split tag
  static const char CFG_DELIMIT_TAG = ',';

  // comments's tag
  static const char CFG_COMMENT_TAG = '#';

  // continue line tag
  static const char CFG_CONTINUE_TAG = '\\';

  // session name tag
  static const char CFG_SESSION_START_TAG = '[';
  static const char CFG_SESSION_END_TAG = ']';

protected:
  /**
   * insert one empty session to sections_
   */
  void insert_session(const std::string &session_name);

  /**
   * switch session according to the session_name
   * if the section doesn't exist, it will create one
   */
  std::map<std::string, std::string> *switch_session(const std::string &session_name);

  /**
   * insert one entry to session_map
   * line's format is "key=value"
   *
   */
  int insert_entry(std::map<std::string, std::string> *session_map, const std::string &line);

  typedef std::map<std::string, std::map<std::string, std::string>> SessionsMap;

private:
  static const std::map<std::string, std::string> empty_map_;

  std::set<std::string> file_names_;
  SessionsMap sections_;
};

/**
 * Global configurate propertis
 */
Ini *&get_properties();
//********************************************************************

}  // namespace common
#endif  //__COMMON_CONF_INI_H__
