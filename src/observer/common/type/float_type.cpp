/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "common/lang/comparator.h"
#include "common/lang/sstream.h"
#include "common/log/log.h"
#include "common/type/float_type.h"
#include "common/value.h"
#include "common/lang/limits.h"
#include "common/value.h"
// -1 表示 left < right 0 表示 left = right 1 表示 left > right INT32_MAX 表示未实现的比较
int FloatType::compare(const Value &left, const Value &right) const
{
  ASSERT(left.attr_type() == AttrType::FLOATS, "left type is not integer");
  ASSERT(right.attr_type() == AttrType::INTS || right.attr_type() == AttrType::FLOATS, "right type is not numeric");
  float left_val  = left.get_float();
  float right_val = right.get_float();
  return common::compare_float((void *)&left_val, (void *)&right_val);
}

RC FloatType::add(const Value &left, const Value &right, Value &result) const
{
  result.set_float(left.get_float() + right.get_float());
  return RC::SUCCESS;
}

RC FloatType::max(const Value &left, const Value &right, Value &result) const{
  result.set_float((left.get_float() > right.get_float())? left.get_float():right.get_float());
  return RC::SUCCESS;
}

RC FloatType::min(const Value &left, const Value &right, Value &result) const{
  result.set_float((left.get_float() < right.get_float())? left.get_float():right.get_float());
  return RC::SUCCESS;
}

// left 新值
RC FloatType::avg(const Value &left, const int num,  const Value &right, Value &result) const{
  float totalSum = right.get_float() * num + left.get_float();
  result.set_float(totalSum / (num + 1));
  return RC::SUCCESS;
}

RC FloatType::subtract(const Value &left, const Value &right, Value &result) const
{
  result.set_float(left.get_float() - right.get_float());
  return RC::SUCCESS;
}
RC FloatType::multiply(const Value &left, const Value &right, Value &result) const
{
  result.set_float(left.get_float() * right.get_float());
  return RC::SUCCESS;
}

RC FloatType::divide(const Value &left, const Value &right, Value &result) const
{
  if (right.get_float() > -EPSILON && right.get_float() < EPSILON) {
    // NOTE:
    // 设置为浮点数最大值是不正确的。通常的做法是设置为NULL，但是当前的miniob没有NULL概念，所以这里设置为浮点数最大值。
    result.set_float(numeric_limits<float>::max());
  } else {
    result.set_float(left.get_float() / right.get_float());
  }
  return RC::SUCCESS;
}

RC FloatType::negative(const Value &val, Value &result) const
{
  result.set_float(-val.get_float());
  return RC::SUCCESS;
}

RC FloatType::set_value_from_str(Value &val, const string &data) const
{
  RC                rc = RC::SUCCESS;
  stringstream deserialize_stream;
  deserialize_stream.clear();
  deserialize_stream.str(data);

  float float_value;
  deserialize_stream >> float_value;
  if (!deserialize_stream || !deserialize_stream.eof()) {
    rc = RC::SCHEMA_FIELD_TYPE_MISMATCH;
  } else {
    val.set_float(float_value);
  }
  return rc;
}

RC FloatType::to_string(const Value &val, string &result) const
{
  stringstream ss;
  ss << common::double_to_str(val.value_.float_value_);
  result = ss.str();
  return RC::SUCCESS;
}

/**
 * @brief 浮点数暂时仅支持转换到 INTS  BOOLEANS FLOATS
 */
int FloatType::cast_cost(AttrType type){     // 浮点 => 其他类型的转换衡量
  if(type == AttrType::MAXTYPE || type == AttrType::UNDEFINED || type == AttrType::DATES || type == AttrType::CHARS){
      return INT32_MAX;
  }
  return 0;
} 

/**
 * 自定义转换逻辑
 * 将 val（自己的类型） 转换为 type 类型，并将结果保存到 result 中
 */
RC FloatType::cast_to(const Value &val, AttrType type, Value &result) const {
    if(val.attr_type() != AttrType::FLOATS){
      LOG_WARN("The type be to cast %s is not matching for the current type %s", val.attr_type(), AttrType::FLOATS);
      return RC::UNSUPPORTED; 
    }
    if(type == AttrType::BOOLEANS){
        result.set_type(AttrType::BOOLEANS);
        result.set_boolean(val.get_float() != 0.0f);
        return RC::SUCCESS;
    }
    else if(type == AttrType::FLOATS){
        result.set_type(AttrType::FLOATS);
        result.set_float(val.get_float());
        return RC::SUCCESS;
    }
    else if(type == AttrType::INTS){        // 注意：浮点数与整数比较默认不发生转换，此处逻辑可能以偏概全，但应该没有其他地方需要涉及浮点与整数的转换
        result.set_type(AttrType::FLOATS);
        result.set_float(val.get_float());
        return RC::SUCCESS;
    }
    else{
      LOG_WARN("can not cast %s to %s", val.attr_type(), type);
      return RC::UNSUPPORTED; 
    }
}