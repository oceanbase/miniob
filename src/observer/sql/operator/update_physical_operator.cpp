#include "sql/operator/update_physical_operator.h"
#include "storage/record/record.h"
#include "storage/record/record_manager.h"
#include "sql/operator/delete_physical_operator.h"
#include "sql/stmt/insert_stmt.h"
#include "storage/table/table.h"
#include "storage/trx/trx.h"

using namespace std;

UpdatePhysicalOperator::UpdatePhysicalOperator(Table *table, Field field,Value value)
    : table_(table), field_(field),value_(value)
{}




RC UpdatePhysicalOperator::open(Trx *trx)
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
/*
RC UpdatePhysicalOperator::next()
{

  RC rc=RC::SUCCESS;
  if (children_.empty()) {
    return RC::RECORD_EOF;
  }


  PhysicalOperator *child = children_[0].get();
  while (RC::SUCCESS == (rc = child->next())) {
    Tuple *tuple = child->current_tuple();
    if (nullptr == tuple) {
      LOG_WARN("failed to get current record: %s", strrc(rc));
      return rc;
    }
    RowTuple *row_tuple = static_cast<RowTuple *>(tuple);
    Record &old_record = row_tuple->record();

    // Construct new record data with updated field
    const std::vector<FieldMeta> *field_metas = table_->table_meta().field_metas();
    const char *target_field_name = field_.field_name();
    size_t field_count = field_metas->size();
    size_t target_index = size_t(-1);
    for (size_t i = 0; i < field_count; ++i) {
      if (strcmp((*field_metas)[i].name(), target_field_name) == 0) {
        target_index = i;
        break;
      }
    }
    if (target_index == size_t(-1)) {
      LOG_WARN("Target field '%s' not found", target_field_name);
      return RC::SCHEMA_FIELD_NOT_EXIST;
    }

    //获取value
    vector<Value> values;
    for(int i=0 ; i < row_tuple->cell_num(); i++){
      Value value;
      tuple->cell_at(i,value);
      values.push_back(value);
    }
    values[target_index]=value_;



    // Delete the old record
    RC rc = trx_->delete_record(table_, old_record);
    if (rc != RC::SUCCESS) {
      LOG_WARN("Failed to delete old record: %s", strrc(rc));
      return rc;
    }

    // Insert the new record
    Record new_record;
    rc = table_->make_record(static_cast<int>(values.size()), values.data(), new_record);

    //new_record.set_data(new_record_data.data(), static_cast<int>(new_record_data.size()));
    rc = trx_->insert_record(table_, new_record);
    if (rc != RC::SUCCESS) {
      LOG_WARN("Failed to insert new record: %s", strrc(rc));
      // Attempt to rollback the deletion
      trx_->insert_record(table_, old_record);
      return rc;
    }
  }

  return RC::RECORD_EOF;
}
*/

//way2
RC UpdatePhysicalOperator::next()
{
  RC rc = RC::SUCCESS;
  if (children_.empty()) {
    return RC::RECORD_EOF;
  }
  PhysicalOperator *child = children_[0].get();


  while (RC::SUCCESS == (rc = child->next())) {
    Tuple *tuple = child->current_tuple();
    if (nullptr == tuple) {
      LOG_WARN("failed to get current record: %s", strrc(rc));
      return rc;
    }

    RowTuple *row_tuple = static_cast<RowTuple *>(tuple);
    Record &record = row_tuple->record();
    const char * field_name=field_.field_name();
    const FieldMeta * field_meta = table_->table_meta().field(field_name);
    int offset_=field_meta->offset();
    int len_=field_meta->len();
    rc = trx_->update_record(table_, record, offset_, len_,value_);

    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to update record: %s", strrc(rc));
      return rc;
    }
  }

  return RC::RECORD_EOF;
}


RC UpdatePhysicalOperator::close()
{
  // 在此处执行任何必要的清理操作。
  if (!children_.empty()) {
    children_[0]->close();
  }
  return RC::SUCCESS;
}




