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

#ifndef COMMON_LINEREADER_H
#define COMMON_LINEREADER_H

#include "common/lang/string.h"

namespace common {
/**
 * @brief A wrapper class for linenoise lib
 * @details Support history memory...
 */
class LineReader {
public:
  LineReader();
  ~LineReader();

  /**
   * @brief Read input
   * @param prompt
   * @return "input" or ""
   */
  std::string input(const std::string& prompt);
  char* input(const char* prompt);
  
  /**
   * @brief Load history records from the file
   * @param history_file path/to/history
   * @return Whether load success
   */
  bool history_load(const std::string& history_file);
  
  /**
   * @brief Save history records to the file
   * @param history_file path/to/history
   * @return Whether save success
   */
  bool history_save(const std::string& history_file) const;
  
  /**
   * @brief Add a single history record to the file
   * @param line the command to be recorded
   * @return Whether add success
   */
  bool history_add(const std::string& line);
  
  /**
   * @brief Set the maximum of history length
   * @param len length
   */
  void history_set_max_len(int len);
  
  /**
   * @brief 安装窗口变化处理程序，用于适配终端大小变化
   */
  void install_window_change_handler();
  
  /**
   * @brief Clean the screen: Ctrl + L
   */
  void clear_screen();

private:
  std::string current_history_file_;
};

/**
 * @brief Check whether a string is all blank
 * @param str given string
 */
bool is_valid_input(const std::string& str);

} // namespace common

#endif // COMMON_LINEREADER_H 

