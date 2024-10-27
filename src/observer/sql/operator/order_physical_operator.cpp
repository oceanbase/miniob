#include "sql/operator/order_physical_operator.h"
#include "common/log/log.h"
#include "sql/stmt/order_stmt.h"
#include "storage/field/field.h"
#include "storage/record/record.h"

class cmp
{
private:
    vector<bool> sortRules; // 多少个SortUnit 就有多少个规则，大于0表示升序
public:
    cmp(vector<bool>& sortRules){
        this->sortRules = sortRules;
    }

    bool operator () (SortTarget* A, SortTarget* B) const {     // whether B should put ahead of A
        for(size_t i = 0; i < sortRules.size(); i++ ){
            // 获得当前排序规则
            bool rule = sortRules.at(i);

            // 取出 A 和 B 当前value
            Value va = A->value.at(i);
            Value vb = B->value.at(i);
            
            // 获得 A 和 B 的类型
            AttrType aa = va.attr_type();
            AttrType ba = vb.attr_type();
            if(aa == AttrType::NULLS && ba != AttrType::NULLS){
                return true;   // B在A后面
            } else if(aa != AttrType::NULLS && ba == AttrType::NULLS){
                return false;    // B在A前面
            } else if(aa == AttrType::NULLS && ba == AttrType::NULLS){
                continue;   // 都为空，则继续判断下一个条件
            } else{
                int cmpRes = va.compare(vb);
                // -1  ->  left < right
                // 0   ->  left = right
                // 1   ->  left > right

                if(cmpRes == 0){ continue; }
                if(cmpRes == 1){ return rule? false:true;}
                return rule? true:false;
            }
        }
        return false;    // 所有条件都等于，保留原顺序
    }
};

RC OtherTuple2ValueListTuple(ValueListTuple*& resTuple, Tuple* oriTuple){
    RC rc = RC::SUCCESS;

    if(oriTuple->getType() == TupleType::ROW_TUPLE){
      oriTuple = static_cast<RowTuple*> (oriTuple);
    }
    else if(oriTuple->getType() == TupleType::JOINED_TUPLE){
      oriTuple = static_cast<JoinedTuple*> (oriTuple);
    }
    else{
      return RC::INTERNAL;
    }

    std::vector<Value> cells;              // 存储字段的值
    std::vector<TupleCellSpec> specs;      // 存储字段的类型
    for(int i = 0; i < oriTuple->cell_num(); i++){
        Value tmp;
        
        rc = oriTuple->cell_at(i,tmp);
        if(rc != RC::SUCCESS){
          LOG_WARN("fail to get value of RowTuple");
          return RC::INTERNAL;
        }

        TupleCellSpec tmp2;
        rc = oriTuple->spec_at(i,tmp2);
        if(rc != RC::SUCCESS){
          LOG_WARN("fail to get tupleCellSpec of RowTuple");
          return RC::INTERNAL;
        }

        cells.push_back(tmp);
        specs.push_back(tmp2);
    }
    resTuple->set_cells(cells);
    resTuple->set_names(specs);
    return RC::SUCCESS;
}

RC OrderPhysicalOperator::open(Trx *trx)
{
  // 1 - 获得排序规则
  vector<bool> sortRules;
  for(OrderUnit* ou: order_units_){
    sortRules.push_back(ou->get_inc_order());
  }

  // 2 - 递归处理子算子，要求保证本层在倒数第二层，即上层有且仅能有 过滤算子
  if (children_.size() != 1) {
    LOG_WARN("order operator must has one child");
    return RC::INTERNAL;
  }

  PhysicalOperator &child = *children_[0];
  RC                rc    = child.open(trx);  
  if (OB_FAIL(rc)) {
    LOG_INFO("failed to open child operator. rc=%s", strrc(rc));
    return rc;
  }

// 记录一条一条扫上来
  while (OB_SUCC(rc = child.next())) { 
    Tuple *child_tuple = child.current_tuple(); 
    if (nullptr == child_tuple) {
      LOG_WARN("failed to get tuple from child operator. rc=%s", strrc(rc));
      return RC::INTERNAL;
    }


    // 3 - 保存原始 tuple， 以 ValueListTuple 的形式
    // TODO 优化：将 ValueListTuple 对象存储在文件之中，在磁盘进行排序 Big Data Order
    ValueListTuple* resTuple = new ValueListTuple();

    if(child_tuple->getType() != TupleType::ROW_TUPLE && child_tuple->getType() != TupleType::JOINED_TUPLE){
      LOG_WARN("order operator tuple must be rowTuple or joinedTuple");
      return RC::INTERNAL;
    }
    else{
      rc = OtherTuple2ValueListTuple(resTuple, child_tuple);
      if(rc != RC::SUCCESS){
        LOG_WARN("Failed to transfer childTuple to rowTuple or joinedTuple");
        return RC::INTERNAL;
      }
    }
    scanned_tuples_.push_back(resTuple);

    // 4 - 筛选出需要排序的值
    vector<Value> values;
    for(OrderUnit* ou: order_units_){
        Expression* fieldExpr = ou->get_bound_field_expr();
        Value curValue;
        rc = fieldExpr->get_value(*child_tuple, curValue);
        if(rc != RC::SUCCESS){
            LOG_WARN("failed to get value from child tuple");
            return RC::INTERNAL;
        }
        values.push_back(curValue);
    }
    // 保存需要排序的值（以封装的对象形式）
    addSortTarget(values, scanned_tuples_.size()-1);
  }

    // 5 - 对所有记录加以排序
    sort(sortTarger_.begin(), sortTarger_.end(), cmp(sortRules));

    // 初始时返回第一个元组
    ResultPtr = -1;

  return RC::SUCCESS;
}

RC OrderPhysicalOperator::next()
{
    // 每一次next，就更新目前的 result_ptr, 指向已排序好的下一个元组
    ResultPtr++;
    return RC::SUCCESS;
}

Tuple *OrderPhysicalOperator::current_tuple() {
    // 返回当前 result_ptr 指向的元组 ; 若无记录返回空  
    if(sortTarger_.size() == 0 || ResultPtr >= int(sortTarger_.size())){
      return NULL;
    }
    int idx = sortTarger_.at(ResultPtr)->scanned_tuples_idx;

    // 获取并返回 ValueListTuple 对象
    return scanned_tuples_.at(idx);
}

RC OrderPhysicalOperator::close()
{
  children_[0]->close();
  return RC::SUCCESS;
}

RC OrderPhysicalOperator::tuple_schema(TupleSchema &schema) const
{
  return children_[0]->tuple_schema(schema);
}

void OrderPhysicalOperator::addSortTarget(vector<Value>& values, int idx){
    SortTarget* sortTarger = new SortTarget();
    sortTarger->scanned_tuples_idx = idx;
    sortTarger->value = values;
    this->sortTarger_.push_back(sortTarger);    //delete
}

