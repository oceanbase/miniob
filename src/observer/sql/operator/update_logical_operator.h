#pragma once

#include "common/lang/string.h"
#include "common/value.h"
#include "sql/operator/logical_operator.h"

class Table;

class UpdateLogicalOperator : public LogicalOperator
{
public:
  UpdateLogicalOperator(Table *table, const char *attribute_name, const Value &value)
      : table_(table), attribute_name_(attribute_name), value_(value)
  {}

  LogicalOperatorType type() const override { return LogicalOperatorType::UPDATE; }
  OpType              get_op_type() const override { return OpType::LOGICALUPDATE; }
  Table              *table() const { return table_; }
  const char         *attribute_name() const { return attribute_name_.c_str(); }
  const Value        &value() const { return value_; }

private:
  Table *table_;
  string attribute_name_;
  Value  value_;
};
