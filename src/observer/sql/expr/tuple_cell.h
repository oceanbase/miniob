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

class TupleCellSpec {
public:
  TupleCellSpec(const char *table_name, const char *field_name, const char *alias = nullptr);
  TupleCellSpec(const char *alias);

  const char *table_name() const
  {
    return table_name_.c_str();
  }
  const char *field_name() const
  {
    return field_name_.c_str();
  }
  const char *alias() const
  {
    return alias_.c_str();
  }

private:
  std::string table_name_;
  std::string field_name_;
  std::string alias_;
};

/**
 * 表示tuple中某个元素的值
 * @note 可以与value做合并
 */
class TupleCell {
public:
  TupleCell() = default;

  TupleCell(FieldMeta *meta, char *data, int length = 4) : TupleCell(meta->type(), data)
  {}
  TupleCell(AttrType attr_type, char *data, int length = 4) : attr_type_(attr_type)
  {
    this->set_data(data, length);
  }

  TupleCell(const TupleCell &other) = default;
  TupleCell &operator=(const TupleCell &other) = default;

  void set_type(AttrType type)
  {
    this->attr_type_ = type;
  }
  void set_data(char *data, int length);
  void set_data(const char *data, int length)
  {
    this->set_data(const_cast<char *>(data), length);
  }
  void set_int(int val);
  void set_float(float val);
  void set_boolean(bool val);
  void set_string(const char *s, int len = 0);
  void set_value(const Value &value);

  void to_string(std::ostream &os) const;

  int compare(const TupleCell &other) const;

  const char *data() const;
  int length() const
  {
    return length_;
  }

  AttrType attr_type() const
  {
    return attr_type_;
  }

public:
  /**
   * 获取对应的值
   * 如果当前的类型与期望获取的类型不符，就会执行转换操作
   */
  int get_int() const;
  float get_float() const;
  std::string get_string() const;
  bool get_boolean() const;

private:
  AttrType attr_type_ = UNDEFINED;
  int length_;

  union {
    int int_value_;
    float float_value_;
    bool bool_value_;
  } num_value_;
  std::string str_value_;
};
