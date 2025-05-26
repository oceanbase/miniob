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

#define MAX_MEM_BUFFER_SIZE 8192
namespace oceanbase {

inline const string LINE_HISTORY_FILE = "./.oblsm_cli.history";

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