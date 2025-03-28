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
// Created by Ping Xu(haibarapink@gmail.com)
//
#pragma once

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <map>
#include <ctime>

#include "common/sys/rc.h"
#include "common/lang/string.h"
#include "common/lang/string_view.h"
#include "oblsm/client/cliutil/defs.h"

#ifdef USE_READLINE
#include "readline/history.h"
#include "readline/readline.h"
#endif

#define MAX_MEM_BUFFER_SIZE 8192
namespace oceanbase {

#ifdef USE_READLINE
inline const string HISTORY_FILE            = string(getenv("HOME")) + "/.oblsm_cli.history";
inline time_t       last_history_write_time = 0;

inline std::string my_readline(const string &prompt)
{
  int size = history_length;
  if (size == 0) {
    read_history(HISTORY_FILE.c_str());

    std::ofstream historyFile(HISTORY_FILE, std::ios::app);
    if (historyFile.is_open()) {
      historyFile.close();
    }
  }

  char       *line = readline(prompt.c_str());
  std::string result;
  if (line != nullptr && line[0] != 0) {
    result = line;
    add_history(line);

    if (std::time(nullptr) - last_history_write_time > 5) {
      write_history(HISTORY_FILE.c_str());
    }
  }
  free(line);
  return result;
}
#else   // USE_READLINE
inline std::string my_readline(const std::string &prompt)
{
  std::cout << prompt;
  std::string buffer;
  if (std::getline(std::cin, buffer)) {
    return buffer;
  } else {
    return "";
  }
}
#endif  // USE_READLINE

enum class TokenType
{
  COMMAND,
  STRING,
  BOUND,
  INVALID
};

class ObLsmCliCmdTokenizer
{
public:
  TokenType       token_type;
  ObLsmCliCmdType cmd;
  string          str;

  ObLsmCliCmdTokenizer()
  {
#define MAP_COMMAND(cmd) token_map_[string{ObLsmCliUtil::strcmd(ObLsmCliCmdType::cmd)}] = ObLsmCliCmdType::cmd
    MAP_COMMAND(OPEN);
    MAP_COMMAND(CLOSE);
    MAP_COMMAND(SET);
    MAP_COMMAND(DELETE);
    MAP_COMMAND(SCAN);
    MAP_COMMAND(HELP);
    MAP_COMMAND(EXIT);
    MAP_COMMAND(GET);
#undef MAP_COMMAND
  }

  void init(string_view command)
  {
    command_ = command;
    p_       = 0;
  }

  RC next();

private:
  bool out_of_range() { return p_ >= command_.size(); }
  void skip_blank_space();
  RC   parse_string(string &res);

  std::map<string, ObLsmCliCmdType> token_map_;

  string_view command_;
  size_t      p_;
};

// Semantic checks
class ObLsmCliCmdParser
{
public:
  struct Result
  {
    ObLsmCliCmdType cmd;
    string          error;

    string args[2];
    bool   bounds[2] = {false, false};
  };
  Result result;
  RC     parse(string_view command);

private:
  ObLsmCliCmdTokenizer tokenizer_;
};

}  // namespace oceanbase