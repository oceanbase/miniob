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

#include "common/linereader/linenoise_reader.h"
#include "common/linereader/linenoise.h"
#include "common/lang/string.h"

LinenoiseReader::LinenoiseReader()
{
  linenoiseHistorySetMaxLen(1000);
  linenoiseSetMultiLine(0);
}

char *LinenoiseReader::input(const char *prompt) { return linenoise(prompt); }

bool LinenoiseReader::history_load(const std::string &history_file)
{
  history_file_ = history_file;
  return linenoiseHistoryLoad(history_file.c_str()) == 0;
}

bool LinenoiseReader::history_save(const std::string &history_file) const
{
  return linenoiseHistorySave(history_file.c_str()) == 0;
}

bool LinenoiseReader::history_add(const char *line)
{
  if (line != nullptr && *line != '\0') {
    return linenoiseHistoryAdd(line) == 0;
  }
  return false;
}

void LinenoiseReader::history_set_max_len(int len) { linenoiseHistorySetMaxLen(len); }

void LinenoiseReader::clear_screen() { linenoiseClearScreen(); }
