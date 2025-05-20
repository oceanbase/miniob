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

#ifndef COMMON_LINE_READER_H
#define COMMON_LINE_READER_H

#include "replxx.hxx"
using LineReader = replxx::Replxx;

namespace common {
class MiniobLineReader
{
public:
  /**
   * @brief Read a line from input
   * @param prompt The prompt to display
   * @param history_file path/to/file
   * @return char* to input string or nullptr
   */
  static std::string my_readline(const std::string &prompt, const std::string &history_file);

  /**
   * @brief Check if the command is an exit command
   * @param cmd The input command
   * @param history_file path/to/file
   * @return True if the command is an exit command
   */
  static bool is_exit_command(const std::string &cmd, const std::string &history_file);

  /**
   * @brief Save history to file
   * @param history_file path/to/file
   * @return True if save succeed
   */
  static bool save_history(const std::string &history_file);

private:
  /**
   * @brief Check if history should be auto-saved
   * @param history_file path/to/file
   * @return True if history should be saved due to time interval
   */
  static bool check_and_save_history(const std::string &history_file);

private:
  static LineReader reader_;
  static bool       is_first_call_;
  static time_t     previous_history_save_time_;
  static int        history_save_interval_;
};
}  // namespace common

#endif  // COMMON_LINE_READER_H
