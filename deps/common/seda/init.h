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

#ifndef __COMMON_SEDA_INIT_H__
#define __COMMON_SEDA_INIT_H__

// Basic includes
#include <assert.h>
#include <signal.h>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "common/conf/ini.h"
#include "common/defs.h"
#include "common/os/process_param.h"
namespace common {

/**
 * start the seda process, do this will trigger all threads
 */
int init_seda(ProcessParam *process_cfg);

void cleanup_seda();

}  // namespace common
#endif  // __COMMON_SEDA_INIT_H__
