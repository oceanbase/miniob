/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "sql/operator/project_vec_physical_operator.h"
#include "common/log/log.h"
#include "storage/record/record.h"
#include "storage/table/table.h"

using namespace std;

ProjectVecPhysicalOperator::ProjectVecPhysicalOperator(vector<unique_ptr<Expression>> &&expressions)
    : expressions_(std::move(expressions))
{
  int expr_pos = 0;
  for (auto &expr : expressions_) {
    chunk_.add_column(make_unique<Column>(expr->value_type(), expr->value_length()), expr_pos);
    expr_pos++;
  }
}
RC ProjectVecPhysicalOperator::open(Trx *trx)
{
  if (children_.empty()) {
    return RC::SUCCESS;
  }
  RC rc = children_[0]->open(trx);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to open child operator: %s", strrc(rc));
    return rc;
  }

  return RC::SUCCESS;
}

RC ProjectVecPhysicalOperator::next(Chunk &chunk)
{
  if (children_.empty()) {
    return RC::RECORD_EOF;
  }
  chunk_.reset_data();
  RC rc = children_[0]->next(chunk_);
  if (rc == RC::RECORD_EOF) {
    return rc;
  } else if (rc == RC::SUCCESS) {
    rc = chunk.reference(chunk_);
  } else {
    LOG_WARN("failed to get next tuple: %s", strrc(rc));
    return rc;
  }
  return rc;
}

RC ProjectVecPhysicalOperator::close()
{
  if (!children_.empty()) {
    children_[0]->close();
  }
  return RC::SUCCESS;
}

RC ProjectVecPhysicalOperator::tuple_schema(TupleSchema &schema) const
{
  for (const unique_ptr<Expression> &expression : expressions_) {
    schema.append_cell(expression->name());
  }
  return RC::SUCCESS;
}
