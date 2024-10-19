#pragma once

#ifndef BOOLEAN_TYPE_H_
#define BOOLEAN_TYPE_H_

#include "common/type/data_type.h"
#include "common/rc.h"

/**
 * @brief 布尔型数据类型
 * @ingroup DataType
 */
class BooleanType : public DataType
{
public:
  BooleanType() : DataType(AttrType::BOOLEANS) {}
  virtual ~BooleanType() = default;

  RC to_string(const Value &val, string &result) const override;
  
  int compare(const Value &left, const Value &right) const override;

  RC cast_to(const Value &val, AttrType type, Value &result) const override;

  int cast_cost(AttrType type) override;
};

#endif