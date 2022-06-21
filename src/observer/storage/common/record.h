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
// Created by Wangyunlai on 2022/5/4.
//

#pragma once

#include <stddef.h>
#include <vector>

#include "rc.h"
#include "storage/common/index_meta.h"
#include "storage/common/field_meta.h"
#include "storage/common/record_manager.h"

class Record
{
public:
  Record() = default;
  ~Record() = default;

  void set_data(char *data);
  char *data();
  const char *data() const;

  void set_rid(const RID &rid);
  RID & rid();
  const RID &rid() const;

  RC set_field_value(const Value &value, int index);
  RC set_field_values(const Value *values, int value_num, int start_index);

private:
  std::vector<FieldMeta> *   fields_ = nullptr;
  RID                        rid_;

  // the data buffer
  // record will not release the memory
  char *                     data_ = nullptr;
};
