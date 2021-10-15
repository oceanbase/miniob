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
// Created by Longda on 2021
//

#include <stdio.h>

#include "common/math/md5.h"
#include "md5_test.h"

using namespace common;

Md5Test::Md5Test() {
  // Auto-generated constructor stub
}

Md5Test::~Md5Test() {
  // Auto-generated destructor stub
}

void Md5Test::string() {
  char buf[512] = "/home/fastdfs/longda";
  unsigned char digest[16] = {0};
  MD5String(buf, digest);
  for (int i = 0; i < 16; i++) {
    printf("%d: %02x %d\n", i, digest[i], digest[i]);
  }
}

int main(int argc, char **argv) {
  Md5Test test;
  test.string();

  return 0;
}
