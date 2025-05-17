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
// Created by Willaaaaaaa in 2025
//

#ifndef COMMON_LINENOISE_WRAPPER_H
#define COMMON_LINENOISE_WRAPPER_H

#include "common/lang/string.h"

/**
 * @brief A wrapper class for linenoise lib
 * @details Support history memory...
 */
class LinenoiseReader
{
public:
  LinenoiseReader();
  ~LinenoiseReader() = default;

  /**
   * @brief Read input with C++ string
   * @param prompt
   * @return char* to input string or nullptr
  //  * @note set prompt to string to be consistent with replxx
   */
  char *input(const char *prompt);

  /**
   * @brief Load history records from the file
   * @param history_file path/to/history
   * @return Whether load success
   */
  bool history_load(const std::string &history_file);

  /**
   * @brief Save history records to the file
   * @param history_file path/to/history
   * @return Whether save success
   */
  bool history_save(const std::string &history_file) const;

  /**
   * @brief Add a single history record to the file
   * @param line the C string command to be recorded
   * @return Whether add success
   */
  bool history_add(const char *line);

  /**
   * @brief Set the maximum of history length
   * @param len length
   */
  void history_set_max_len(int len);

  /**
   * @brief Clean the screen: Ctrl + L
   */
  void clear_screen();

private:
  std::string history_file_;
};

#endif  // COMMON_LINENOISE_WRAPPER_H
