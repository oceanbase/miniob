#include "sql/operator/aggregate_physical_operator.h"
#include "storage/record/record.h"
#include "storage/table/table.h"
#include "storage/trx/trx.h"
#include "sql/stmt/select_stmt.h"
#include "sql/parser/value.h"
#include "common/lang/comparator.h"

RC AggregatePhysicalOperator::open(Trx *trx)
{
  if (children_.size() != 1) {
    LOG_WARN("aggregate operator must have one child");
    return RC::INTERNAL;
  }

  return children_[0]->open(trx);
}

RC AggregatePhysicalOperator::next()
{
    if(result_tuple_.cell_num() > 0){
        return RC::RECORD_EOF;
    }

    RC rc = RC::SUCCESS;
    PhysicalOperator *oper = children_[0].get();

    std::vector<Value> result_cells;
    int count = 0;
    while(RC::SUCCESS == (rc = oper->next())){
        Tuple *tuple = oper->current_tuple();
        count++;
        for(int cell_idx = 0;cell_idx < (int)aggregations_.size();cell_idx++){
            const AggrOp aggregation = aggregations_[cell_idx];
            Value cell;
            AttrType attr_type = AttrType::INTS;
            rc = tuple->cell_at(cell_idx,cell);
            attr_type = cell.attr_type();
            if(count == 1)
            {
                switch(attr_type){
                    case AttrType::INTS:
                        result_cells.push_back(Value(0));
                        break;
                    case AttrType::FLOATS:
                        result_cells.push_back(Value((float)0));
                        break;
                    case AttrType::CHARS:
                        result_cells.push_back(Value(""));
                        break;
                    default:
                        return RC::UNIMPLENMENT;
                        break;
                }  
            }
            switch(aggregation){
                case AggrOp::AGGR_MIN:
                    if(count == 1)
                    {
                        result_cells[cell_idx] = cell;
                    }
                    else if(cell.compare(result_cells[cell_idx]) < 0)
                    {
                        result_cells[cell_idx] = cell;
                    }
                    break;
                case AggrOp::AGGR_MAX:
                    if(count == 1)
                    {
                        result_cells[cell_idx] = cell;
                    }
                    else if(cell.compare(result_cells[cell_idx]) > 0)
                    {
                        result_cells[cell_idx] = cell;
                    }
                    break;
                case AggrOp::AGGR_AVG:
                case AggrOp::AGGR_SUM:
                    if(attr_type == AttrType::INTS || attr_type == AttrType::FLOATS){
                        result_cells[cell_idx].set_float(result_cells[cell_idx].get_float() + cell.get_float());
                        // result_cells.push_back()
                    }
                    break;
                case AggrOp::AGGR_COUNT_ALL:
                case AggrOp::AGGR_COUNT:
                    result_cells[cell_idx].set_int(count);
                    break;
                default:
                    return RC::UNIMPLENMENT;
            }
        }
    }

    if(rc == RC::RECORD_EOF){
        rc = RC::SUCCESS;
    }

    for (int cell_idx = 0; cell_idx < result_cells.size(); cell_idx++) {
        const AggrOp aggr = aggregations_[cell_idx];
        if (aggr == AggrOp::AGGR_AVG) {
            result_cells[cell_idx].set_float(result_cells[cell_idx].get_float() / count);
            LOG_TRACE("update avg. avg=%s", result_cells[cell_idx].to_string().c_str());
        }
        if ((aggr == AggrOp::AGGR_MAX || aggr == AggrOp::AGGR_MIN) && result_cells[cell_idx].attr_type() == AttrType::CHARS)
        {
            std::string str = result_cells[cell_idx].get_string();
            for(int i = 0;i < str.length();i++)
            {
                if(str[i] >= 97 && str[i] <= 122)
                {
                    str[i] -= 32;
                }
            }
             const char* cstr = str.c_str();
             result_cells[cell_idx].set_string(cstr);
        }
    }

    result_tuple_.set_cells(result_cells);

    return rc;
}

// RC AggregatePhysicalOperator::close()
// {
//   if (!children_.empty()) {
//     children_[0]->close();
//   }
//   return RC::SUCCESS;
// }

RC AggregatePhysicalOperator::close()
{
  return children_[0]->close();
}




