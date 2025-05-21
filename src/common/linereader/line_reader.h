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

namespace common {
/**
 * @brief A singleton class for line-reading, providing an interface to replxx。
 */
class MiniobLineReader
{
  // The following private methods guarantee it's Singleton pattern
private:
  MiniobLineReader();
  ~MiniobLineReader();
  MiniobLineReader(const MiniobLineReader &)            = delete;
  MiniobLineReader &operator=(const MiniobLineReader &) = delete;

public:
  /**
   * @brief Get the singleton instance
   * @return Reference to the singleton instance
   */
  static MiniobLineReader &instance();

  /**
   * @brief Initialize the lineReader with history file
   * @param history_file path/to/file
   */
  void init(const std::string &history_file);

  /**
   * @brief Read a line from input
   * @param prompt The prompt to display
   * @return input string
   */
  std::string my_readline(const std::string &prompt);

  /**
   * @brief Check if the command is an exit command
   * @param cmd The input command
   * @return True if the command is an exit command
   */
  bool is_exit_command(const std::string &cmd);

private:
  /**
   * @brief Check if history should be auto-saved
   * @return True if history should be saved due to time interval
   */
  bool check_and_save_history();

private:
  replxx::Replxx reader_;
  std::string    history_file_;
  time_t         previous_history_save_time_;
  int            history_save_interval_;
};
}  // namespace common

#endif  // COMMON_LINE_READER_H
