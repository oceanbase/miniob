/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "common/log/log.h"
#include "sql/operator/expr_vec_physical_operator.h"
#include "sql/expr/expression_tuple.h"
#include "sql/expr/composite_tuple.h"

using namespace std;
using namespace common;

ExprVecPhysicalOperator::ExprVecPhysicalOperator(std::vector<Expression *> &&expressions)
{
  expressions_ = std::move(expressions);
}

RC ExprVecPhysicalOperator::open(Trx *trx)
{
  ASSERT(children_.size() == 1, "group by operator only support one child, but got %d", children_.size());

  PhysicalOperator &child = *children_[0];
  RC                rc    = child.open(trx);
  if (OB_FAIL(rc)) {
    LOG_INFO("failed to open child operator. rc=%s", strrc(rc));
    return rc;
  }
  return rc;
}

RC ExprVecPhysicalOperator::next(Chunk &chunk)
{
  RC rc = RC::SUCCESS;
  ASSERT(children_.size() == 1, "group by operator only support one child, but got %d", children_.size());

  PhysicalOperator &child = *children_[0];
  chunk.reset();
  evaled_chunk_.reset();
  if (OB_SUCC(rc = child.next(chunk_))) {
    for (size_t i = 0; i < expressions_.size(); i++) {
      auto column = std::make_unique<Column>();
      expressions_[i]->get_column(chunk_, *column);
      evaled_chunk_.add_column(std::move(column), i);
    }
    chunk.reference(evaled_chunk_);
  }
  return rc;
}

RC ExprVecPhysicalOperator::close()
{
  children_[0]->close();
  LOG_INFO("close group by operator");
  return RC::SUCCESS;
}