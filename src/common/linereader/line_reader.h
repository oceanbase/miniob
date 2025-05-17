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

#ifndef COMMON_LINE_READER
#define COMMON_LINE_READER

#if USE_REPLXX
#include "replxx.hxx"
using LineReader = replxx::Replxx;
#else
#include "common/linereader/linenoise_reader.h"
using LineReader = LinenoiseReader;
#endif

namespace common {
class LineReaderManager
{
public:
  /**
   * @brief Read a line from input
   * @param prompt The prompt to display
   * @param history_file path/to/file
   * @return char* to input string or nullptr
   */
  static char *my_readline(const char *prompt, const std::string &history_file);

  /**
   * @brief Check if the command is an exit command
   * @param cmd The input command
   * @param history_file path/to/file
   * @return True if the command is an exit command
   */
  static bool is_exit_command(const char *cmd, const std::string &history_file);

  /**
   * @brief Free the buffer allocated by my_readline
   * @param buffer The buffer allocated by my_readline
   * @note dealing with alloc-dealloc-mismatch
   */
  static void free_buffer(char *buffer);

private:
  static LineReader reader_;
  static bool       is_first_call_;
};
}  // namespace common

#endif  // COMMON_LINE_READER
