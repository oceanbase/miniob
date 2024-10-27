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
// Created by Wangyunlai on 2024/05/29.
//

#include "sql/expr/aggregator.h"
#include "common/log/log.h"

RC SumAggregator::accumulate(const Value &value)
{
  if (value_.attr_type() == AttrType::UNDEFINED) {
    value_ = value;
    return RC::SUCCESS;
  }
  
  ASSERT(value.attr_type() == AttrType::NULLS || value.attr_type() == value_.attr_type(), "type mismatch. value type: %s, value_.type: %s", 
        attr_type_to_string(value.attr_type()), attr_type_to_string(value_.attr_type()));
  
  if(value.attr_type() == AttrType::NULLS){
    return RC::SUCCESS;
  }
  
  Value::add(value, value_, value_);
  return RC::SUCCESS;
}

RC SumAggregator::evaluate(Value& result)
{
  result = value_;
  return RC::SUCCESS;
}



RC MaxAggregator::accumulate(const Value &value)
{
  if (value_.attr_type() == AttrType::UNDEFINED) {
    value_ = value;
    return RC::SUCCESS;
  }
  
  ASSERT(value.attr_type() == AttrType::NULLS || value.attr_type() == value_.attr_type(), "type mismatch. value type: %s, value_.type: %s", 
        attr_type_to_string(value.attr_type()), attr_type_to_string(value_.attr_type()));
  
  /* 核心步骤 */
  if(value.attr_type() == AttrType::NULLS){
    return RC::SUCCESS;
  }
  Value::max(value, value_, value_);

  return RC::SUCCESS;
}

RC MaxAggregator::evaluate(Value& result)
{
  result = value_;
  return RC::SUCCESS;
}


RC MinAggregator::accumulate(const Value &value)
{
  if (value_.attr_type() == AttrType::UNDEFINED) {
    value_ = value;
    return RC::SUCCESS;
  }
  
  ASSERT(value.attr_type() == AttrType::NULLS || value.attr_type() == value_.attr_type(), "type mismatch. value type: %s, value_.type: %s", 
        attr_type_to_string(value.attr_type()), attr_type_to_string(value_.attr_type()));
  
  /* 核心步骤 */
  if(value.attr_type() == AttrType::NULLS){
    return RC::SUCCESS;
  }
  Value::min(value, value_, value_);

  return RC::SUCCESS;
}

RC MinAggregator::evaluate(Value& result)
{
  result = value_;
  return RC::SUCCESS;
}



RC AvgAggregator::accumulate(const Value &value)
{
  // 仅支持 int 和 float 计算平均值
  ASSERT(value.attr_type() == AttrType::NULLS || value.attr_type() == AttrType::FLOATS || value.attr_type() == AttrType::INTS, "AVG operating should be float or int, false type %s", 
        attr_type_to_string(value.attr_type()));
  
  if(num_ == 0){
    // 第一次计算，均把结果类型设为float
    value_.set_type(AttrType::FLOATS);
  }
  else if (value_.attr_type() == AttrType::UNDEFINED) {
    value_ = value;
    return RC::SUCCESS;
  }

  // 不能声明 value 与 value_ 类型一致，后者为浮点

  /* 核心步骤 */
  /* 内部的 num_ 记录当前是第几个value */
  /* 内部的 value_ 表示当前 num_ 个值的平均值 */
  if(value.attr_type() == AttrType::NULLS){   // 不参与平均值计算
    return RC::SUCCESS;
  }

  Value::avg(value, this->num_, value_, value_);
  this->num_++;
  return RC::SUCCESS;
}

RC AvgAggregator::evaluate(Value& result)
{
  result = value_;
  return RC::SUCCESS;
}


RC CountAggregator::accumulate(const Value &value)
{
  // 不能声明value与value_类型一致，无论什么非空类型，value_均为int
  /* 核心步骤 */
  if(value.attr_type() == AttrType::NULLS)  {   // 不发生任何变化
    return RC::SUCCESS;   
  }
  
  this->num_++;
  Value::count(this->num_, value_); 
  LOG_INFO("%s", value_.to_string().data());
  return RC::SUCCESS;
}

RC CountAggregator::evaluate(Value& result)
{
  result = value_;
  return RC::SUCCESS;
}


