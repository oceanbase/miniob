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

#include "common/linereader/linereader.h"
#include "common/linereader/linenoise.h"
#include "common/lang/string.h"

namespace common {

LineReader::LineReader() {
  linenoiseHistorySetMaxLen(1000);
  linenoiseSetMultiLine(0);
}

LineReader::~LineReader() {
  if (!current_history_file_.empty()) {
    history_save(current_history_file_);
  }
}

std::string LineReader::input(const std::string& prompt) {
  char* line = linenoise(prompt.c_str());
  
  if (line == nullptr) {
    return "";
  }
  
  std::string result = line;
  linenoiseFree(line);
  return result;
}

char* LineReader::input(const char* prompt) {
  char* line = linenoise(prompt);
  
  if (line == nullptr) {
    return nullptr;
  }
  
  char* result = strdup(line);
  if (result == nullptr) {
    return nullptr;
  }

  linenoiseFree(line);
  return result;
}

bool LineReader::history_load(const std::string& history_file) {
  current_history_file_ = history_file;
  return linenoiseHistoryLoad(history_file.c_str()) == 0;
}

bool LineReader::history_save(const std::string& history_file) const {
  return linenoiseHistorySave(history_file.c_str()) == 0;
}

bool LineReader::history_add(const std::string& line) {
  if (!line.empty()) {
    return linenoiseHistoryAdd(line.c_str()) == 0;
  }
  return false;
}

void LineReader::history_set_max_len(int len) {
  linenoiseHistorySetMaxLen(len);
}

void LineReader::install_window_change_handler() {}

void LineReader::clear_screen() {
  linenoiseClearScreen();
}

bool is_valid_input(const std::string& str) {
  for (size_t i = 0; i < str.length(); ++i) {
    if (!isspace(str[i])) {
      return true;
    }
  }
  return false;
}

} // namespace common 
