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
// Created by WangYunlai on 2022/6/7.
//

#pragma once

#include <iostream>
#include "storage/common/table.h"
#include "storage/common/field_meta.h"

class TupleCell
{
public: 
  TupleCell() = default;
  
  TupleCell(FieldMeta *meta, char *data)
    : TupleCell(meta->type(), data)
  {}
  TupleCell(AttrType attr_type, char *data)
    : attr_type_(attr_type), data_(data)
  {}

  void set_type(AttrType type) { this->attr_type_ = type; }
  void set_length(int length) { this->length_ = length; }
  void set_data(char *data) { this->data_ = data; }
  void set_data(const char *data) { this->set_data(const_cast<char *>(data)); }

  void to_string(std::ostream &os) const;

  int compare(const TupleCell &other) const;

  const char *data() const
  {
    return data_;
  }

  int length() const { return length_; }

  AttrType attr_type() const
  {
    return attr_type_;
  }

private:
  AttrType attr_type_ = UNDEFINED;
  int length_ = -1;
  char *data_ = nullptr; // real data. no need to move to field_meta.offset
};
