#include "sql/operator/update_physical_operator.h"
#include "sql/stmt/insert_stmt.h"
#include "storage/table/table.h"
#include "storage/trx/trx.h"

using namespace std;

UpdatePhysicalOperator::UpdatePhysicalOperator(Table *table, Field field, Value value)
    : table_(table), field_(field), value_(value)
{}

RC UpdatePhysicalOperator::open(Trx *trx)
{
  if (children_.empty()) {
    return RC::SUCCESS;
  }  

  std::unique_ptr<PhysicalOperator> &child = children_[0];
  RC rc = child->open(trx);

  if (rc != RC::SUCCESS){
    LOG_WARN("failed to open child operator: %s", strrc(rc));
    return rc;
  }

  trx_ = trx;

  return RC::SUCCESS;
}

RC UpdatePhysicalOperator::next()
{
  RC rc = RC::SUCCESS;
  if (children_.empty()){
    return RC::RECORD_EOF;
  }

  PhysicalOperator *child = children_[0].get();
  while (RC::SUCCESS == (rc = child->next())){
    Tuple *tuple = child->current_tuple();
    if (nullptr == tuple){
      LOG_WARN("failed to get current record: %s", strrc(rc));
      return rc;
    }
    RowTuple *row_tuple = static_cast<RowTuple *>(tuple);
    Record &record = row_tuple->record();

    // rc = trx_->update_record(table_, record);
    // 修改record
    rc = trx_->delete_record(table_, record);
    if (rc != RC::SUCCESS){
      LOG_WARN("failed to delete record: %s", strrc(rc));
      return rc;
    }else{
      // 定位列名索引
      const std::vector<FieldMeta> *table_field_metas = table_->table_meta().field_metas();
      const char *target_field_name= field_.field_name();

      int meta_num = table_field_metas->size();
      int target_index = -1;
      for (int i=0; i<meta_num; ++i){
        FieldMeta fieldmeta = (*table_field_metas)[i];
        const char *field_name = fieldmeta.name();
        if (0 == strcmp(field_name, target_field_name)){
          target_index = i;
          break;
        }
      }
      // 重新构造record
      // 1. Values
      std::vector<Value> values;
      int cell_num = row_tuple->cell_num();

      //看懂下面的代码尝试自己写出来
      for (int i=0; i < cell_num; ++i){
        Value cell;
        if (target_index == i){
          cell.set_value(value_);

        }
        else{
          row_tuple->cell_at(i, cell);
        }
        values.emplace_back(cell);
      }
      // 2. Record
      Record new_record;
      RC rc = table_->make_record(cell_num, values.data(), new_record);
      if (rc != RC::SUCCESS) {
        return rc;
      }
      rc = trx_->insert_record(table_, new_record);
      if (rc != RC::SUCCESS) {
        LOG_WARN("failed to insert record: %s", strrc(rc));
        return rc;
      }
    }
  }

  return RC::RECORD_EOF;

}

RC UpdatePhysicalOperator::close()
{
  if (!children_.empty()) {
    children_[0]->close();
  }
  return RC::SUCCESS;
}