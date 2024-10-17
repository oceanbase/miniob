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

#include "common/lang/array.h"
#include "common/lang/memory.h"
#include "common/lang/string.h"
#include "common/rc.h"
#include "common/type/attr_type.h"

class Value;

/**
 * @brief 定义了数据类型相关的操作，比如比较运算、算术运算等
 * @defgroup DataType
 * @details 数据类型定义的算术运算中，比如 add、subtract 等，将按照当前数据类型设置最终结果值的类型。
 * 参与运算的参数类型不一定相同，不同的类型进行运算是否能够支持需要参考各个类型的实现。
 */
class DataType
{
public:
  explicit DataType(AttrType attr_type) : attr_type_(attr_type) {}

  virtual ~DataType() = default;

  inline static DataType *type_instance(AttrType attr_type)
  {
    return type_instances_.at(static_cast<int>(attr_type)).get();
  }

  inline AttrType get_attr_type() const { return attr_type_; }

  /**
   * @return
   *  -1 表示 left < right
   *  0 表示 left = right
   *  1 表示 left > right
   *  INT32_MAX 表示未实现的比较
   */
  virtual int compare(const Value &left, const Value &right) const { return INT32_MAX; }

  /**
   * @brief 计算 left + right，并将结果保存到 result 中
   */
  virtual RC add(const Value &left, const Value &right, Value &result) const { return RC::UNSUPPORTED; }

  /**
   * @brief 计算 left - right，并将结果保存到 result 中
   */
  virtual RC subtract(const Value &left, const Value &right, Value &result) const { return RC::UNSUPPORTED; }

  /**
   * @brief 计算 left * right，并将结果保存到 result 中
   */
  virtual RC multiply(const Value &left, const Value &right, Value &result) const { return RC::UNSUPPORTED; }

  /**
   * @brief 计算 left / right，并将结果保存到 result 中
   */
  virtual RC divide(const Value &left, const Value &right, Value &result) const { return RC::UNSUPPORTED; }

  /**
   * @brief 计算 -val，并将结果保存到 result 中
   */
  virtual RC negative(const Value &val, Value &result) const { return RC::UNSUPPORTED; }

  /**
   * @brief 将 val 转换为 type 类型，并将结果保存到 result 中
   */
  virtual RC cast_to(const Value &val, AttrType type, Value &result) const { return RC::UNSUPPORTED; }

  /**
   * @brief 将 val 转换为 string，并将结果保存到 result 中
   */
  virtual RC to_string(const Value &val, string &result) const { return RC::UNSUPPORTED; }

  /**
   * @brief 计算从 type 到 attr_type 的隐式转换的 cost，如果无法转换，返回 INT32_MAX
   */
  virtual int cast_cost(AttrType type)
  {
    if (type == attr_type_) {
      return 0;
    }
    return INT32_MAX;
  }

  virtual RC set_value_from_str(Value &val, const string &data) const { return RC::UNSUPPORTED; }

protected:
  AttrType attr_type_;

  static array<unique_ptr<DataType>, static_cast<int>(AttrType::MAXTYPE)> type_instances_;
};
