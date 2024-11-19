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
// Created by Longda on 2010
//

#include <regex.h>
#include <stdlib.h>
#include <sys/types.h>

#include "common/math/regex.h"
namespace common {

int regex_match(const char *str_, const char *pat_)
{
  regex_t reg;
  if (regcomp(&reg, pat_, REG_EXTENDED | REG_NOSUB))
    return -1;

  int ret = regexec(&reg, str_, 0, NULL, 0);
  regfree(&reg);
  return ret;
}

}  // namespace common