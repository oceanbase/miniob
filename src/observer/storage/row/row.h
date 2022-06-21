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
// Created by Wangyunlai on 2022/5/3.
//

#ifndef __OBSERVER_STORAGE_ROW_ROW_H_
#define __OBSERVER_STORAGE_ROW_ROW_H_

#include <stddef.h>
#include <vector>

#include "rc.h"
#include "storage/common/index_meta.h"
#include "storage/common/field_meta.h"
#include "storage/common/record_manager.h"

class Row {

public:
  Row() = default;
  ~Row() = default;

  RC init(const FieldMeta *fields_, int num);

  RC set_projector();

private:
  std::vector<FieldMeta *> fields_;
  std::vector<int> projector_;
  char *data_;
};

#endif  // __OBSERVER_STORAGE_ROW_ROW_H_
