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
// Created by Longda on 2010
//

#ifndef __COMMON_VERSION_H__
#define __COMMON_VERSION_H__
namespace common {

#ifndef MAIJOR_VER
#define MAIJOR_VER 1
#endif

#ifndef MINOR_VER
#define MINOR_VER 0
#endif

#ifndef PATCH_VER
#define PATCH_VER 0
#endif

#ifndef OTHER_VER
#define OTHER_VER 1
#endif

#define STR1(R) #R
#define STR2(R) STR1(R)

#define VERSION_STR                                                            \
  (STR2(MAIJOR_VER) "." STR2(MINOR_VER) "." STR2(PATCH_VER) "." STR2(OTHER_VER))
#define VERSION_NUM                                                            \
  (MAIJOR_VER << 24 | MINOR_VER << 16 | PATCH_VER << 8 | OTHER_VER)

} //namespace common
#endif //__COMMON_VERSION_H__
