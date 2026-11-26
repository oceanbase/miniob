#include "sql/operator/update_physical_operator.h"

#include <algorithm>
#include <memory>

#include "common/value.h"
#include "sql/expr/tuple.h"
#include "storage/field/field_meta.h"
#include "storage/record/record.h"
#include "storage/table/table.h"
#include "storage/trx/trx.h"

UpdatePhysicalOperator::UpdatePhysicalOperator(Table *table, const FieldMeta *field, const Value &value)
    : table_(table), field_(field), value_(value)
{}

RC UpdatePhysicalOperator::open(Trx *trx)
{
  if (children_.empty() || table_ == nullptr || field_ == nullptr) {
    return RC::INVALID_ARGUMENT;
  }

  auto &child = children_.front();
  RC    rc    = child->open(trx);
  if (rc != RC::SUCCESS) {
    return rc;
  }

  vector<std::unique_ptr<Record>> records;
  while (OB_SUCC(rc = child->next())) {
    Tuple *tuple = child->current_tuple();
    if (tuple == nullptr) {
      rc = RC::INTERNAL;
      break;
    }

    RowTuple               *row_tuple = static_cast<RowTuple *>(tuple);
    const Record           &source    = row_tuple->record();
    std::unique_ptr<Record> record(new Record());

    rc = record->copy_data(source.data(), source.len());
    if (rc != RC::SUCCESS) {
      break;
    }

    record->set_rid(source.rid());
    records.emplace_back(std::move(record));
  }

  child->close();
  if (rc == RC::RECORD_EOF) {
    rc = RC::SUCCESS;
  }
  if (rc != RC::SUCCESS) {
    return rc;
  }

  for (auto &old_record_ptr : records) {
    Record &old_record = *old_record_ptr;
    Record  new_record;

    rc = new_record.copy_data(old_record.data(), old_record.len());
    if (rc != RC::SUCCESS) {
      return rc;
    }

    new_record.set_rid(old_record.rid());
    rc = new_record.reset_filed(field_->offset(), field_->len());
    if (rc != RC::SUCCESS) {
      return rc;
    }

    int copy_len = field_->type() == AttrType::CHARS ? std::min(field_->len(), value_.length() + 1) : field_->len();
    if (copy_len > value_.length()) {
      return RC::SCHEMA_FIELD_TYPE_MISMATCH;
    }

    rc = new_record.set_field(field_->offset(), copy_len, value_.data());
    if (rc != RC::SUCCESS) {
      return rc;
    }

    rc = trx->update_record(table_, old_record, new_record);
    if (rc != RC::SUCCESS) {
      return rc;
    }
  }

  return RC::SUCCESS;
}