/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by MiniOB Update Implementor.
//

#include "sql/operator/update_physical_operator.h"
#include "common/log/log.h"
#include "sql/expr/tuple.h"
#include "storage/table/table.h"
#include "storage/table/table_meta.h"
#include "storage/trx/trx.h"
#include <string.h>

RC UpdatePhysicalOperator::set_value_to_record(char *record_data, const Value &value, const FieldMeta *field)
{
  size_t copy_len = field->len();
  const size_t data_len = value.length();
  if (field->type() == AttrType::CHARS) {
    if (copy_len > data_len) {
      copy_len = data_len + 1; // include '\0'
    }
  }
  memcpy(record_data + field->offset(), value.data(), copy_len);
  return RC::SUCCESS;
}

RC UpdatePhysicalOperator::open(Trx *trx)
{
  if (children_.empty()) {
    return RC::SUCCESS;
  }

  trx_ = trx;

  unique_ptr<PhysicalOperator> &child = children_[0];

  RC rc = child->open(trx);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to open child operator: %s", strrc(rc));
    return rc;
  }

  while (OB_SUCC(rc = child->next())) {
    Tuple *tuple = child->current_tuple();
    if (nullptr == tuple) {
      LOG_WARN("failed to get current tuple");
      rc = RC::INTERNAL;
      break;
    }

    RowTuple *row_tuple = static_cast<RowTuple *>(tuple);
    Record   &record    = row_tuple->record();
    records_.emplace_back(std::move(record));
  }

  child->close();

  if (rc != RC::SUCCESS && rc != RC::RECORD_EOF) {
    return rc;
  }

  const TableMeta &meta = table_->table_meta();
  const int rec_size = meta.record_size();

  // delete + insert to update rows
  for (Record &old_rec : records_) {
    Record new_rec;
    rc = new_rec.copy_data(old_rec.data(), rec_size);
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to copy record. rc=%s", strrc(rc));
      return rc;
    }

    Value write_val = value_;
    if (write_val.attr_type() != field_->type()) {
      Value casted;
      rc = Value::cast_to(write_val, field_->type(), casted);
      if (OB_FAIL(rc)) {
        LOG_WARN("failed to cast value. rc=%s", strrc(rc));
        return rc;
      }
      write_val = std::move(casted);
    }

    rc = set_value_to_record(const_cast<char *>(new_rec.data()), write_val, field_);
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to set value to record. rc=%s", strrc(rc));
      return rc;
    }

    rc = trx_->delete_record(table_, old_rec);
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to delete record. rc=%s", strrc(rc));
      return rc;
    }

    rc = trx_->insert_record(table_, new_rec);
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to insert record. rc=%s", strrc(rc));
      return rc;
    }
  }

  return RC::SUCCESS;
}

RC UpdatePhysicalOperator::next()
{
  return RC::RECORD_EOF;
}

RC UpdatePhysicalOperator::close()
{
  return RC::SUCCESS;
}


