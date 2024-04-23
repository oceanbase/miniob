
#include "common/log/log.h"
#include "storage/record/record.h"
#include "sql/stmt/filter_stmt.h"
#include "storage/field/field.h"
#include "aggregate_physical_operator.h"
#include "common/log/log.h"
#include "storage/table/table.h"
#include "storage/trx/trx.h"

RC AggregatePhysicalOperator::open(Trx *trx)
{
  if (children_.empty()) {
    return RC::SUCCESS;
  }

  std::unique_ptr<PhysicalOperator> &child = children_[0];
  RC                                 rc    = child->open(trx);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to open child operator: %s", strrc(rc));
    return rc;
  }

  return RC::SUCCESS;
}

RC AggregatePhysicalOperator::next()
{
    // already aggregated
    if (result_tuple_.cell_num() > 0){
        return RC::RECORD_EOF;
    }

    RC rc = RC::SUCCESS;
    PhysicalOperator *oper = children_[0].get();
    
    std::vector<Value> result_cells;
    int tuple_num = 0;//tuple数量
    while (RC::SUCCESS == (rc = oper->next())) {

        Tuple *tuple = oper->current_tuple();
        tuple_num++;

        for (int cell_idx = 0; cell_idx < (int)aggregations_.size(); cell_idx++) {
            const AggrOp aggregation = aggregations_[cell_idx];
            
            Value cell;
            AttrType attr_type = AttrType::INTS;

            switch (aggregation){

            case AggrOp::AGGR_SUM:
                rc = tuple->cell_at(cell_idx, cell);
                attr_type = cell.attr_type();
                if(attr_type == AttrType::INTS or attr_type == AttrType::FLOATS) {
                  if(static_cast<int>(result_cells.size())!=(int)aggregations_.size()){
                    result_cells.push_back(cell);
                  }else{
                    result_cells[cell_idx].set_float(result_cells[cell_idx].get_float() + cell.get_float());
                  }
                }
                break;

            case AggrOp::AGGR_AVG:
                rc = tuple->cell_at(cell_idx, cell);
                attr_type = cell.attr_type();
                if(attr_type == AttrType::INTS or attr_type == AttrType::FLOATS) {
                  if(static_cast<int>(result_cells.size())!=(int)aggregations_.size()){
                    result_cells.push_back(cell);
                  }else{
                    result_cells[cell_idx].set_float(result_cells[cell_idx].get_float() + cell.get_float());
                  }
                }
                break;

            case AggrOp::AGGR_MAX:
                rc = tuple->cell_at(cell_idx, cell);
                if(static_cast<int>(result_cells.size())!=(int)aggregations_.size()){
                    result_cells.push_back(cell);
                }else{
                  int flag = cell.compare(result_cells[cell_idx]);
                  if(flag > 0)//cell.value>result_cells[cell_idx].value
                    result_cells[cell_idx].set_value(cell);
                  }
                break;

            case AggrOp::AGGR_MIN:
                rc = tuple->cell_at(cell_idx, cell);
                if(static_cast<int>(result_cells.size())!=(int)aggregations_.size()){
                    result_cells.push_back(cell);
                }else{
                  int flag = cell.compare(result_cells[cell_idx]);
                  if(flag < 0)//cell.value < result_cells[cell_idx].value
                    result_cells[cell_idx].set_value(cell);
                  }           
                break;

            case AggrOp::AGGR_COUNT:
                rc = tuple->cell_at(cell_idx, cell);
                if(static_cast<int>(result_cells.size())!=(int)aggregations_.size()){
                  result_cells.push_back(cell);
              }
                break;
            case AggrOp::AGGR_COUNT_ALL:
                rc = tuple->cell_at(cell_idx, cell);
                if(static_cast<int>(result_cells.size())!=(int)aggregations_.size()){
                  result_cells.push_back(cell);
              }
                break;
            default:
                return RC::UNIMPLENMENT;
            }
        }
    }

    for (int cell_idx = 0; cell_idx < (int)aggregations_.size(); cell_idx++) {
        const AggrOp aggregation = aggregations_[cell_idx];

        if(aggregation == AggrOp::AGGR_AVG){
          float new_float=result_cells[cell_idx].get_float()/tuple_num;
          result_cells[cell_idx].set_float(new_float);
        }
        
        if(aggregation == AggrOp::AGGR_COUNT || aggregation == AggrOp::AGGR_COUNT_ALL){
          result_cells[cell_idx].set_float((float)tuple_num);
        }
      }   
    
    if (rc == RC::RECORD_EOF){
        rc = RC::SUCCESS;
    }
    result_tuple_.set_cells(result_cells);

    return rc;
}

RC AggregatePhysicalOperator::close()
{
  if (!children_.empty()) {
    children_[0]->close();
  }
  return RC::SUCCESS;
}

void AggregatePhysicalOperator::add_aggregation(const AggrOp aggregation){
    aggregations_.push_back(aggregation);
}
Tuple *AggregatePhysicalOperator::current_tuple()
{
    return &result_tuple_;
}