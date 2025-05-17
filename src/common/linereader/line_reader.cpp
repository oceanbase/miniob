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

#include "common/linereader/line_reader.h"
#include "common/lang/string.h"

namespace common {
LineReader LineReaderManager::reader_;
bool       LineReaderManager::is_first_call_ = true;

char *LineReaderManager::my_readline(const char *prompt, const std::string &history_file)
{
  if (is_first_call_) {
    reader_.history_load(history_file);
    is_first_call_ = false;
  }

  char *line = (char *)reader_.input(prompt);
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
    reader_.history_add(line);
  }

  return line;
}

bool LineReaderManager::is_exit_command(const char *cmd, const std::string &history_file)
{
  bool is_exit = 0 == strncasecmp("exit", cmd, 4) || 0 == strncasecmp("bye", cmd, 3) ||
                 0 == strncasecmp("\\q", cmd, 2) || 0 == strncasecmp("interrupted", cmd, 11);

  if (is_exit) {
    reader_.history_save(history_file);
  }

  return is_exit;
}

void LineReaderManager::free_buffer(char *buffer)
{
  if (buffer != nullptr) {
#if USE_REPLXX
    delete[] buffer;  // replxx uses new[]
#else
    free(buffer);  // linenoise uses malloc
#endif
  }
}
}  // namespace common
