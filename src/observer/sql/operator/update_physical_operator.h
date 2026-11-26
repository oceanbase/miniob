#pragma once

#include "common/value.h"
#include "sql/operator/physical_operator.h"

class Table;
class Trx;
class FieldMeta;
class Value;

class UpdatePhysicalOperator : public PhysicalOperator
{
public:
  UpdatePhysicalOperator(Table *table, const FieldMeta *field, const Value &value);
  PhysicalOperatorType type() const override { return PhysicalOperatorType::UPDATE; }
  OpType               get_op_type() const override { return OpType::UPDATE; }
  RC                   open(Trx *trx) override;
  RC                   next() override { return RC::RECORD_EOF; }
  RC                   close() override { return RC::SUCCESS; }

private:
  Table           *table_;
  const FieldMeta *field_;
  Value            value_;
};
