/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "sql/operator/table_scan_vec_physical_operator.h"
#include "event/sql_debug.h"
#include "storage/table/table.h"

using namespace std;

RC TableScanVecPhysicalOperator::open(Trx *trx)
{
  RC rc = table_->get_chunk_scanner(chunk_scanner_, trx, mode_);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to get chunk scanner", strrc(rc));
    return rc;
  }
  // TODO: don't need to fetch all columns from record manager
  for (int i = 0; i < table_->table_meta().field_num(); ++i) {
    all_columns_.add_column(
        make_unique<Column>(*table_->table_meta().field(i)), table_->table_meta().field(i)->field_id());
    filterd_columns_.add_column(
        make_unique<Column>(*table_->table_meta().field(i)), table_->table_meta().field(i)->field_id());
  }
  return rc;
}

RC TableScanVecPhysicalOperator::next(Chunk &chunk)
{
  RC rc = RC::SUCCESS;

  all_columns_.reset_data();
  filterd_columns_.reset_data();
  if (OB_SUCC(rc = chunk_scanner_.next_chunk(all_columns_))) {
    select_.assign(all_columns_.rows(), 1);
    if (predicates_.empty()) {
      chunk.reference(all_columns_);
    } else {
      rc = filter(all_columns_);
      if (rc != RC::SUCCESS) {
        LOG_TRACE("filtered failed=%s", strrc(rc));
        return rc;
      }
      // TODO: if all setted, it doesn't need to set one by one
      for (int i = 0; i < all_columns_.rows(); i++) {
        if (select_[i] == 0) {
          continue;
        }
        for (int j = 0; j < all_columns_.column_num(); j++) {
          filterd_columns_.column(j).append_one(
              (char *)all_columns_.column(filterd_columns_.column_ids(j)).get_value(i).data());
        }
      }
      chunk.reference(filterd_columns_);
    }
  }
  return rc;
}

RC TableScanVecPhysicalOperator::close() { return chunk_scanner_.close_scan(); }

string TableScanVecPhysicalOperator::param() const { return table_->name(); }

void TableScanVecPhysicalOperator::set_predicates(vector<unique_ptr<Expression>> &&exprs)
{
  predicates_ = std::move(exprs);
}

RC TableScanVecPhysicalOperator::filter(Chunk &chunk)
{
  RC rc = RC::SUCCESS;
  for (unique_ptr<Expression> &expr : predicates_) {
    rc = expr->eval(chunk, select_);
    if (rc != RC::SUCCESS) {
      return rc;
    }
  }
  return rc;
}
