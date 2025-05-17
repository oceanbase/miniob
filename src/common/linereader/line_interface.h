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
// Created by Willaaaaaaa on 2025
//

#ifndef COMMON_LINE_INTERFACE_H
#define COMMON_LINE_INTERFACE_H

#if USE_REPLXX
  #include "replxx.hxx"
  using LineInterface = replxx::Replxx;
#else
  #include "common/linereader/linereader.h"
  using LineInterface = common::LineReader;
#endif

static LineInterface reader;

/**
 * @brief Read a line from input
 * @param prompt The prompt to display
 * @param history_file path/to/file
 * @return char* to input string or nullptr
 */
inline char* my_readline(const char* prompt, const std::string& history_file) {
  static bool is_first_call = true;

  if (is_first_call) {
    reader.history_load(history_file);
    is_first_call = false;
  }

  char* line = reader.input(prompt);
  if (line == nullptr) {
    return nullptr;
  }

  bool is_valid_input = false;
  for (auto c = line; *c != '\0'; ++c) {
    if (!isspace(*c)) {
      is_valid_input = true;
      break;
    }
  }

  if (is_valid_input) {
    reader.history_add(line);
  }
  
  return line;
}

/**
 * @brief Check if the command is an exit command
 * @param cmd The input command
 * @param history_file path/to/file
 * @return True if the command is an exit command
 */
inline bool is_exit_command(const char* cmd, const std::string& history_file) {
  bool is_exit = 0 == strncasecmp("exit", cmd, 4) || 
                 0 == strncasecmp("bye", cmd, 3) || 
                 0 == strncasecmp("\\q", cmd, 2) ||
                 0 == strncasecmp("interrupted", cmd, 11);
                 
  if (is_exit) {
    reader.history_save(history_file);
  }
  
  return is_exit;
}

#endif // COMMON_LINE_INTERFACE_H
