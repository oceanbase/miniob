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

#include "process_param.h"
#include <assert.h>
namespace common {

//! Global process config
ProcessParam *&the_process_param()
{
  static ProcessParam *process_cfg = new ProcessParam();

  return process_cfg;
}

void ProcessParam::init_default(std::string &process_name)
{
  assert(process_name.empty() == false);
  this->process_name_ = process_name;
  if (std_out_.empty()) {
    std_out_ = "../log/" + process_name + ".out";
  }
  if (std_err_.empty()) {
    std_err_ = "../log/" + process_name + ".err";
  }
  if (conf.empty()) {
    conf = "../etc/" + process_name + ".ini";
  }

  demon = false;
}

}  // namespace common