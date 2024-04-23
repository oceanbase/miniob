#pragma once
#include <memory>
#include <vector>
#include"sql/expr/expression.h"
#include"storage/field/field.h"
#include"sql/operator/logical_operator.h"
class AggregateLogicalOperator: public LogicalOperator
{
    public:
    AggregateLogicalOperator(const std::vector<Field> &field);  
    virtual ~AggregateLogicalOperator() = default;

    LogicalOperatorType type() const override{
        return LogicalOperatorType::AGGREGATE;
    }

    const std::vector<Field> &fields() const{
        return fields_;
    }

    private:
    std::vector<Field> fields_;

};