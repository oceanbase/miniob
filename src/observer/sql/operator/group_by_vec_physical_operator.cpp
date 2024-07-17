/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "sql/operator/group_by_vec_physical_operator.h"


RC GroupByVecPhysicalOperator::open(Trx *trx) 
{ 
  ASSERT(children_.size() == 1, "group by operator only support one child, but got %d", children_.size());

  PhysicalOperator &child = *children_[0];
  RC                rc    = child.open(trx);
  if (OB_FAIL(rc)) { 
    LOG_INFO("failed to open child operator. rc=%s", strrc(rc));
    return rc;
  }

  while (OB_SUCC(rc = child.next(chunk_))) {
    // 每当拿到一条新的子chunk
    // 根据group_by_expr 计算子chunk 的 value
    // 根据该value更新哈希，并返回哈希对应的位置
    // 根据子chunk 计算aggr_expr的 value
    // 放入哈希表对应的位置。

    // 在本函数中，使用group_by_exprs拿出group_chunk，使用aggr_exprs拿出aggr_chunk;
    Chunk group_chunk;
    Chunk aggregate_chunk;
    for (size_t group_by_idx = 0; group_by_idx < group_by_exprs_.size(); ++group_by_idx){
      std::unique_ptr<Column> column_1 = std::make_unique<Column>(group_by_exprs_[group_by_idx]->value_type(),group_by_exprs_[group_by_idx]->value_length());
      group_by_exprs_[group_by_idx]->get_column(chunk_, *column_1);
      group_chunk.add_column(std::move(column_1), group_by_exprs_[group_by_idx]->pos());
      output_chunk_.add_column(make_unique<Column>(group_by_exprs_[group_by_idx]->value_type(),group_by_exprs_[group_by_idx]->value_length()), group_by_idx);
    }

    for (size_t aggr_idx = 0; aggr_idx < value_expressions_.size(); aggr_idx++) {
      std::unique_ptr<Column> column_2 = std::make_unique<Column>(value_expressions_[aggr_idx]->value_type(),value_expressions_[aggr_idx ]->value_length());
      value_expressions_[aggr_idx]->get_column(chunk_, *column_2);
      ASSERT(aggregate_expressions_[aggr_idx]->type() == ExprType::AGGREGATION, "expect aggregate expression");
      aggregate_chunk.add_column(std::move(column_2), value_expressions_[aggr_idx]->pos());
      output_chunk_.add_column(make_unique<Column>(value_expressions_[aggr_idx]->value_type(),value_expressions_[aggr_idx ]->value_length()), aggr_idx + group_by_exprs_.size());
    }
    hashtable->add_chunk(group_chunk, aggregate_chunk);
  }

  hashtable_scanner->open_scan();
  return RC::SUCCESS;
}

RC GroupByVecPhysicalOperator::next(Chunk &chunk)
{
  output_chunk_.reset_data();
  chunk.reset();
  RC rc = hashtable_scanner->next(output_chunk_);
  chunk.reference(output_chunk_);
  return rc;
}   


RC GroupByVecPhysicalOperator::close()
{
  children_[0]->close();
  LOG_INFO("close group by operator");
  return RC::SUCCESS;
}