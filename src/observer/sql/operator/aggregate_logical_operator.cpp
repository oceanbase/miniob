#include "sql/operator/aggregate_logical_operator.h"
#include <vector>
AggregateLogicalOperator::AggregateLogicalOperator(const std::vector<Field> &field):fields_(field){};
