/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
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
#include "rc.h"
#include "common/log/log.h"

RC parse(char *st, Command *sqln);

const char *ATTR_TYPE_NAME[] = {"undefined", "chars", "ints", "floats", "booleans"};

const char *attr_type_to_string(AttrType type)
{
  if (type >= UNDEFINED && type <= FLOATS) {
    return ATTR_TYPE_NAME[type];
  }
  return "unknown";
}
AttrType attr_type_from_string(const char *s)
{
  for (unsigned int i = 0; i < sizeof(ATTR_TYPE_NAME) / sizeof(ATTR_TYPE_NAME[0]); i++) {
    if (0 == strcmp(ATTR_TYPE_NAME[i], s)) {
      return (AttrType)i;
    }
  }
  return UNDEFINED;
}

const char *Value::data() const
{
  switch (type) {
    case INTS:
      return (const char *)&int_value;
    case FLOATS:
      return (const char *)&float_value;
    case BOOLEANS:
      return (const char *)&bool_value;
    case CHARS:
      return (const char *)string_value.data();
    case UNDEFINED:
      return nullptr;
  }
  return nullptr;
}

int Value::length()
{
  switch (type) {
    case INTS:
      return sizeof(int_value);
    case FLOATS:
      return sizeof(float_value);
    case BOOLEANS:
      return sizeof(bool_value);
    case CHARS:
      return string_value.size();
    case UNDEFINED:
      return 0;
  }
  return 0;
}

Command::Command() : flag(SCF_ERROR)
{}

Command::Command(enum SqlCommandFlag _flag) : flag(_flag)
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
