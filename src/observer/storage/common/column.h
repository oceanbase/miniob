/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#pragma once

#include <string.h>

#include "storage/field/field_meta.h"
#include "storage/common/vector_buffer.h"

/**
 * @brief A column contains multiple values in contiguous memory with a specified type.
 */
// TODO: `Column` currently only support fixed-length type.
class Column
{
public:
  enum class Type
  {
    NORMAL_COLUMN,   /// Normal column represents a list of fixed-length values
    CONSTANT_COLUMN  /// Constant column represents a single value
  };

  Column() = default;

  Column(const Column &other)
  {
    count_       = other.count_;
    capacity_    = other.capacity_;
    own_         = true;
    attr_type_   = other.attr_type_;
    attr_len_    = other.attr_len_;
    column_type_ = other.column_type_;
    data_        = new char[capacity_ * attr_len_];
    memcpy(data_, other.data_, capacity_ * attr_len_);
    vector_buffer_ = make_unique<VectorBuffer>();
  }
  Column(Column &&other)
  {
    data_           = other.data_;
    count_          = other.count_;
    capacity_       = other.capacity_;
    own_            = other.own_;
    attr_type_      = other.attr_type_;
    attr_len_       = other.attr_len_;
    column_type_    = other.column_type_;
    vector_buffer_  = std::move(other.vector_buffer_);
    other.data_     = nullptr;
    other.count_    = 0;
    other.capacity_ = 0;
    other.own_      = false;
  }

  Column(const FieldMeta &meta, size_t size = DEFAULT_CAPACITY);
  Column(AttrType attr_type, int attr_len, size_t size = DEFAULT_CAPACITY);

  void init(const FieldMeta &meta, size_t size = DEFAULT_CAPACITY);
  void init(AttrType attr_type, int attr_len, size_t size = DEFAULT_CAPACITY);
  void init(const Value &value, size_t size);

  unique_ptr<Column> clone() const { return make_unique<Column>(*this); }

  virtual ~Column() { reset(); }

  void reset();

  RC append_one(const char *data);

  RC append_value(const Value &val);

  /**
   * @brief 向 Column 追加写入数据
   * @param data 要被写入数据的起始地址
   * @param count 要写入数据的长度（这里指列值的个数，而不是字节）
   */
  RC append(const char *data, int count);

  /**
   * @brief 获取 index 位置的列值
   */
  Value get_value(int index) const;

  RC copy_to(void *dest, int start_rows, int insert_rows) const
  {
    memcpy(dest, data_ + start_rows * attr_len_, insert_rows * attr_len_);
    return RC::SUCCESS;
  }

  /**
   * @brief 获取列数据的实际大小（字节）
   */
  int data_len() const { return count_ * attr_len_; }

  char *data() const { return data_; }

  string_t add_text(const char *str, int len);

  /**
   * @brief 重置列数据，但不修改元信息
   */
  void reset_data()
  {
    count_         = 0;
    vector_buffer_ = nullptr;
  }

  /**
   * @brief 引用另一个 Column
   */
  void reference(const Column &column);

  void set_column_type(Type column_type) { column_type_ = column_type; }
  void set_attr_type(AttrType attr_type) { attr_type_ = attr_type; }
  void set_count(int count) { count_ = count; }

  int                     count() const { return count_; }
  int                     capacity() const { return capacity_; }
  AttrType                attr_type() const { return attr_type_; }
  int                     attr_len() const { return attr_len_; }
  Type                    column_type() const { return column_type_; }
  static constexpr size_t DEFAULT_CAPACITY = 8192;

private:
  char *data_ = nullptr;
  /// 当前列值数量
  int count_ = 0;
  /// 当前容量，count_ <= capacity_
  int capacity_ = 0;
  /// 是否拥有内存
  bool own_ = true;
  /// 列属性类型
  AttrType attr_type_ = AttrType::UNDEFINED;
  /// 列属性类型长度（目前只支持定长）
  int attr_len_ = -1;
  /// 列类型
  Type                     column_type_   = Type::NORMAL_COLUMN;
  unique_ptr<VectorBuffer> vector_buffer_ = nullptr;
};
