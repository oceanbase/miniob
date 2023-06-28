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
// Created by Meiyi
//

#include <mutex>
#include "sql/parser/parse.h"
#include "common/log/log.h"

RC parse(char *st, Command *sqln);

Command::Command() : flag(SCF_ERROR)
{}

Command::Command(SqlCommandFlag _flag) : flag(_flag)
{}

void ParsedSqlResult::add_command(std::unique_ptr<Command> command)
{
  sql_commands_.emplace_back(std::move(command));
}

////////////////////////////////////////////////////////////////////////////////

int sql_parse(const char *st, ParsedSqlResult *sql_result);

RC parse(const char *st, ParsedSqlResult *sql_result)
{
  sql_parse(st, sql_result);
  return RC::SUCCESS;
}
