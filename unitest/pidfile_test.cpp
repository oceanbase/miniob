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
// Created by Longda on 2021/4/16.
//
#include <unistd.h>

#include "gtest/gtest.h"

#include "common/os/pidfile.h"
#include "common/io/io.h"
#include "common/lang/string.h"

using namespace common;

int main() {
  long long pid = (long long)getpid();

  const char *programName = "test";
  writePidFile(programName);

  std::string pidFile = getPidPath();

  char buf[1024] = {0};
  char *p = buf;
  size_t size = 0;
  readFromFile(pidFile, p, size);

  std::string temp(p);
  long long target = 0;
  str_to_val(temp, target);

  EXPECT_EQ(pid, target);
}