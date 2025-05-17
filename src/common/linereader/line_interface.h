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
// Created by Willaaaaaaa on 2025
//

#ifndef COMMON_LINE_INTERFACE_H
#define COMMON_LINE_INTERFACE_H

#if USE_REPLXX
  #include "replxx.hxx"
  using LineInterface = replxx::Replxx;
#else
  #include "common/linereader/linereader.h"
  using LineInterface = common::LineReader;
#endif

#endif // COMMON_LINE_INTERFACE_H
