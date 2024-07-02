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
#include "common/lang/iostream.h"
#include "common/lang/string.h"
#include "common/lang/utility.h"
#include "common/lang/fstream.h"
#include "common/log/log.h"

namespace common {

const string              Ini::DEFAULT_SECTION = string("");
const map<string, string> Ini::empty_map_;

Ini::Ini() {}

Ini::~Ini() {}

void Ini::insert_session(const string &session_name)
{
  map<string, string>               session_map;
  pair<string, map<string, string>> entry = pair<string, map<string, string>>(session_name, session_map);

  sections_.insert(entry);
}

map<string, string> *Ini::switch_session(const string &session_name)
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

const map<string, string> &Ini::get(const string &section)
{
  SessionsMap::iterator it = sections_.find(section);
  if (it == sections_.end()) {
    return empty_map_;
  }

  return it->second;
}

string Ini::get(const string &key, const string &defaultValue, const string &section)
{
  map<string, string> section_map = get(section);

  map<string, string>::iterator it = section_map.find(key);
  if (it == section_map.end()) {
    return defaultValue;
  }

  return it->second;
}

int Ini::put(const string &key, const string &value, const string &section)
{
  map<string, string> *section_map = switch_session(section);

  section_map->insert(pair<string, string>(key, value));

  return 0;
}

int Ini::insert_entry(map<string, string> *session_map, const string &line)
{
  if (session_map == nullptr) {
    cerr << __FILE__ << __FUNCTION__ << " session map is null" << endl;
    return -1;
  }
  size_t equal_pos = line.find_first_of('=');
  if (equal_pos == string::npos) {
    cerr << __FILE__ << __FUNCTION__ << "Invalid configuration line " << line << endl;
    return -1;
  }

  string key   = line.substr(0, equal_pos);
  string value = line.substr(equal_pos + 1);

  strip(key);
  strip(value);

  session_map->insert(pair<string, string>(key, value));

  return 0;
}

int Ini::load(const string &file_name)
{
  ifstream ifs;

  try {

    bool continue_last_line = false;

    map<string, string> *current_session = switch_session(DEFAULT_SECTION);

    char line[MAX_CFG_LINE_LEN];

    string line_entry;

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
        string session_name            = string(read_buf + 1);

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
    cout << "Successfully load " << file_name << endl;
  } catch (...) {
    if (ifs.is_open()) {
      ifs.close();
    }
    cerr << "Failed to load " << file_name << SYS_OUTPUT_ERROR << endl;
    return -1;
  }

  return 0;
}

void Ini::to_string(string &output_str)
{
  output_str.clear();

  output_str += "Begin dump configuration\n";

  for (SessionsMap::iterator it = sections_.begin(); it != sections_.end(); it++) {
    output_str += CFG_SESSION_START_TAG;
    output_str += it->first;
    output_str += CFG_SESSION_END_TAG;
    output_str += "\n";

    map<string, string> &section_map = it->second;

    for (map<string, string>::iterator sub_it = section_map.begin(); sub_it != section_map.end(); sub_it++) {
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
