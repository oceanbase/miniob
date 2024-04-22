#include "common/log/log.h"
#include "sql/operator/aggregate_physical_operator.h"
#include "storage/record/record.h"
#include "storage/table/table.h"
#include "storage/trx/trx.h"
#include "sql/stmt/delete_stmt.h"
#include <iostream>


void AggregatePhysicalOperator:: add_aggregation(const AggrOp aggregation)
{
  aggregations_.push_back(aggregation);
}
RC AggregatePhysicalOperator::open(Trx *trx)
{
  if (children_.empty()) {
    return RC::SUCCESS;
  }

  std::unique_ptr<PhysicalOperator> &child = children_[0];
  RC rc = child->open(trx);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to open child operator: %s", strrc(rc));
    return rc;
  }

  trx_ = trx;

  return RC::SUCCESS;
}
RC AggregatePhysicalOperator::next()
{
    if (result_tuple_.cell_num() > 0) {
        return RC::RECORD_EOF;
    }

    RC rc = RC::SUCCESS;
    PhysicalOperator *oper = children_[0].get();

    std::vector<Value> result_cells(aggregations_.size(), Value());
    std::vector<int> count(aggregations_.size(), 0); 

    while (RC::SUCCESS == (rc = oper->next())) {
        Tuple *tuple = oper->current_tuple();
        for (int cell_idx = 0; cell_idx < (int)aggregations_.size(); cell_idx++) {
            const AggrOp aggregation = aggregations_[cell_idx];

            Value cell;
            if (tuple->cell_at(cell_idx, cell) != RC::SUCCESS) {
                continue; 
            }

            switch (aggregation) {
            case AggrOp::AGGR_SUM:
            case AggrOp::AGGR_AVG: 
                if (cell.attr_type() == AttrType::INTS || cell.attr_type() == AttrType::FLOATS) {
                    if (result_cells[cell_idx].attr_type() == AttrType::UNDEFINED) {
                        result_cells[cell_idx] = cell;
                        result_cells[cell_idx].set_type(cell.attr_type()); 
                    } else {
                        result_cells[cell_idx].set_float(result_cells[cell_idx].get_float() + cell.get_float());
                    }
                    if (aggregation == AggrOp::AGGR_AVG) {
                        count[cell_idx]++; 
                    }
                }
                break;
            case AggrOp::AGGR_MAX:
                rc = tuple->cell_at(cell_idx, cell);
                if (result_cells[cell_idx].attr_type() == AttrType::UNDEFINED) {
                    result_cells[cell_idx] = cell;
                    result_cells[cell_idx].set_type(cell.attr_type()); 
                } else {
                    if (cell.compare(result_cells[cell_idx]) > 0) {
                        result_cells[cell_idx].set_value(cell);
                    }
                }
                break;
            case AggrOp::AGGR_MIN:
                rc = tuple->cell_at(cell_idx, cell);
                if (static_cast<int>(result_cells.size()) != (int)aggregations_.size()) {
                    result_cells.push_back(cell);
                } else {
                    if (cell.compare(result_cells[cell_idx]) < 0) {
                    result_cells[cell_idx].set_value(cell);
                    }
                }
                break;
            case AggrOp::AGGR_COUNT:
            case AggrOp::AGGR_COUNT_ALL:
                count[cell_idx]++;
                break;
            default:
                return RC::UNIMPLENMENT;
            }
        }
    }
    if (rc == RC::RECORD_EOF) {
        rc = RC::SUCCESS;
        for (int i = 0; i < (int)aggregations_.size(); i++) {
            if (aggregations_[i] == AggrOp::AGGR_AVG && count[i] > 0) {
                result_cells[i].set_float(result_cells[i].get_float() / count[i]);
            } else if (aggregations_[i] == AggrOp::AGGR_COUNT||aggregations_[i] == AggrOp::AGGR_COUNT_ALL) {
                result_cells[i].set_int(count[i]);
                result_cells[i].set_type(AttrType::INTS);
            }
        }
        
       result_tuple_.set_cells(result_cells); 
    }

    return rc;
}

RC AggregatePhysicalOperator::close()
{
  if (!children_.empty()) {
    children_[0]->close();
  }
  return RC::SUCCESS;
}
Tuple *AggregatePhysicalOperator::current_tuple()
{
    return &result_tuple_;
}