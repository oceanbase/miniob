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

#include <errno.h>
#include <string.h>

#include <fstream>

#include "common/conf/ini.h"
#include "common/defs.h"
#include "common/lang/string.h"
#include "common/log/log.h"

namespace common {

const std::string Ini::DEFAULT_SECTION = std::string("");
const std::map<std::string, std::string> Ini::empty_map_;

Ini::Ini()
{}

Ini::~Ini()
{}

void Ini::insert_session(const std::string &session_name)
{
  std::map<std::string, std::string> session_map;
  std::pair<std::string, std::map<std::string, std::string>> entry =
      std::pair<std::string, std::map<std::string, std::string>>(session_name, session_map);

  sections_.insert(entry);
}

std::map<std::string, std::string> *Ini::switch_session(const std::string &session_name)
{
  SessionsMap::iterator it = sections_.find(session_name);
  if (it != sections_.end()) {
    return &it->second;
  }

  insert_session(session_name);

  it = sections_.find(session_name);
  if (it != sections_.end()) {
    return &it->second;
  }

  // should never go this
  return nullptr;
}

const std::map<std::string, std::string> &Ini::get(const std::string &section)
{
  SessionsMap::iterator it = sections_.find(section);
  if (it == sections_.end()) {
    return empty_map_;
  }

  return it->second;
}

std::string Ini::get(const std::string &key, const std::string &defaultValue, const std::string &section)
{
  std::map<std::string, std::string> section_map = get(section);

  std::map<std::string, std::string>::iterator it = section_map.find(key);
  if (it == section_map.end()) {
    return defaultValue;
  }

  return it->second;
}

int Ini::put(const std::string &key, const std::string &value, const std::string &section)
{
  std::map<std::string, std::string> *section_map = switch_session(section);

  section_map->insert(std::pair<std::string, std::string>(key, value));

  return 0;
}

int Ini::insert_entry(std::map<std::string, std::string> *session_map, const std::string &line)
{
  if (session_map == nullptr) {
    std::cerr << __FILE__ << __FUNCTION__ << " session map is null" << std::endl;
    return -1;
  }
  size_t equal_pos = line.find_first_of('=');
  if (equal_pos == std::string::npos) {
    std::cerr << __FILE__ << __FUNCTION__ << "Invalid configuration line " << line << std::endl;
    return -1;
  }

  std::string key = line.substr(0, equal_pos);
  std::string value = line.substr(equal_pos + 1);

  strip(key);
  strip(value);

  session_map->insert(std::pair<std::string, std::string>(key, value));

  return 0;
}

int Ini::load(const std::string &file_name)
{
  std::ifstream ifs;

  try {

    bool continue_last_line = false;

    std::map<std::string, std::string> *current_session = switch_session(DEFAULT_SECTION);

    char line[MAX_CFG_LINE_LEN];

    std::string line_entry;

    ifs.open(file_name.c_str());
    while (ifs.good()) {

      memset(line, 0, sizeof(line));

      ifs.getline(line, sizeof(line));

      char *read_buf = strip(line);

      if (strlen(read_buf) == 0) {
        // empty line
        continue;
      }

      if (read_buf[0] == CFG_COMMENT_TAG) {
        // comments line
        continue;
      }

      if (read_buf[0] == CFG_SESSION_START_TAG && read_buf[strlen(read_buf) - 1] == CFG_SESSION_END_TAG) {

        read_buf[strlen(read_buf) - 1] = '\0';
        std::string session_name = std::string(read_buf + 1);

        current_session = switch_session(session_name);

        continue;
      }

      if (continue_last_line == false) {
        // don't need continue last line
        line_entry = read_buf;
      } else {
        line_entry += read_buf;
      }

      if (read_buf[strlen(read_buf) - 1] == CFG_CONTINUE_TAG) {
        // this line isn't finished, need continue
        continue_last_line = true;

        // remove the last character
        line_entry = line_entry.substr(0, line_entry.size() - 1);
        continue;
      } else {
        continue_last_line = false;
        insert_entry(current_session, line_entry);
      }
    }
    ifs.close();

    file_names_.insert(file_name);
    std::cout << "Successfully load " << file_name << std::endl;
  } catch (...) {
    if (ifs.is_open()) {
      ifs.close();
    }
    std::cerr << "Failed to load " << file_name << SYS_OUTPUT_ERROR << std::endl;
    return -1;
  }

  return 0;
}

void Ini::to_string(std::string &output_str)
{
  output_str.clear();

  output_str += "Begin dump configuration\n";

  for (SessionsMap::iterator it = sections_.begin(); it != sections_.end(); it++) {
    output_str += CFG_SESSION_START_TAG;
    output_str += it->first;
    output_str += CFG_SESSION_END_TAG;
    output_str += "\n";

    std::map<std::string, std::string> &section_map = it->second;

    for (std::map<std::string, std::string>::iterator sub_it = section_map.begin(); sub_it != section_map.end();
         sub_it++) {
      output_str += sub_it->first;
      output_str += "=";
      output_str += sub_it->second;
      output_str += "\n";
    }
    output_str += "\n";
  }

  output_str += "Finish dump configuration \n";

  return;
}

//! Accessor function which wraps global properties object
Ini *&get_properties()
{
  static Ini *properties = new Ini();
  return properties;
}

}  // namespace common
