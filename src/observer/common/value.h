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
// Created by Wangyunlai 2023/6/27
//

#pragma once

#include "common/lang/string.h"
#include "common/lang/memory.h"
#include "common/type/attr_type.h"
#include "common/type/data_type.h"

/**
 * @brief 属性的值
 * @ingroup DataType
 * @details 与DataType，就是数据类型，配套完成各种算术运算、比较、类型转换等操作。这里同时记录了数据的值与类型。
 * 当需要对值做运算时，建议使用类似 Value::add 的操作而不是 DataType::add。在进行运算前，应该设置好结果的类型，
 * 比如进行两个INT类型的除法运算时，结果类型应该设置为FLOAT。
 */
class Value final
{
public:
  friend class DataType;
  friend class IntegerType;
  friend class FloatType;
  friend class BooleanType;
  friend class CharType;

  Value() = default;

  ~Value() { reset(); }

  Value(AttrType attr_type, char *data, int length = 4) : attr_type_(attr_type) { this->set_data(data, length); }

  explicit Value(int val);
  explicit Value(float val);
  explicit Value(bool val);
  explicit Value(const char *s, int len = 0);

  Value(const Value &other);
  Value(Value &&other);

  Value &operator=(const Value &other);
  Value &operator=(Value &&other);

  void reset();

  static RC add(const Value &left, const Value &right, Value &result)
  {
    return DataType::type_instance(result.attr_type())->add(left, right, result);
  }

  static RC subtract(const Value &left, const Value &right, Value &result)
  {
    return DataType::type_instance(result.attr_type())->subtract(left, right, result);
  }

  static RC multiply(const Value &left, const Value &right, Value &result)
  {
    return DataType::type_instance(result.attr_type())->multiply(left, right, result);
  }

  static RC divide(const Value &left, const Value &right, Value &result)
  {
    return DataType::type_instance(result.attr_type())->divide(left, right, result);
  }

  static RC negative(const Value &value, Value &result)
  {
    return DataType::type_instance(result.attr_type())->negative(value, result);
  }

  static RC cast_to(const Value &value, AttrType to_type, Value &result)
  {
    return DataType::type_instance(value.attr_type())->cast_to(value, to_type, result);
  }

  void set_type(AttrType type) { this->attr_type_ = type; }
  void set_data(char *data, int length);
  void set_data(const char *data, int length) { this->set_data(const_cast<char *>(data), length); }
  void set_value(const Value &value);
  void set_boolean(bool val);

  string to_string() const;

  int compare(const Value &other) const;

  const char *data() const;

  int      length() const { return length_; }
  AttrType attr_type() const { return attr_type_; }

public:
  /**
   * 获取对应的值
   * 如果当前的类型与期望获取的类型不符，就会执行转换操作
   */
  int    get_int() const;
  float  get_float() const;
  string get_string() const;
  bool   get_boolean() const;

private:
  void set_int(int val);
  void set_float(float val);
  void set_string(const char *s, int len = 0);
  void set_string_from_other(const Value &other);

private:
  AttrType attr_type_ = AttrType::UNDEFINED;
  int      length_    = 0;

  union Val
  {
    int32_t int_value_;
    float   float_value_;
    bool    bool_value_;
    char   *pointer_value_;
  } value_ = {.int_value_ = 0};

  /// 是否申请并占有内存, 目前对于 CHARS 类型 own_data_ 为true, 其余类型 own_data_ 为false
  bool own_data_ = false;
};
