/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include <benchmark/benchmark.h>

#include "common/lang/memory.h"
#include "common/lang/vector.h"
#include "sql/expr/aggregate_hash_table.h"

class AggregateHashTableBenchmark : public benchmark::Fixture
{
public:
  void SetUp(const ::benchmark::State &state) override
  {
    unique_ptr<Column> column1 = make_unique<Column>(AttrType::INTS, 4);
    unique_ptr<Column> column2 = make_unique<Column>(AttrType::INTS, 4);
    for (int i = 0; i < state.range(0); i++) {
      int key = i % 8;
      column1->append_one((char *)&key);
      column2->append_one((char *)&i);
    }
    group_chunk_.add_column(std::move(column1), 0);
    aggr_chunk_.add_column(std::move(column2), 0);
  }

  void TearDown(const ::benchmark::State &state) override
  {
    group_chunk_.reset();
    aggr_chunk_.reset();
  }

protected:
  Chunk group_chunk_;
  Chunk aggr_chunk_;
};

class DISABLED_StandardAggregateHashTableBenchmark : public AggregateHashTableBenchmark
{
public:
  void SetUp(const ::benchmark::State &state) override
  {

    AggregateHashTableBenchmark::SetUp(state);
    AggregateExpr        aggregate_expr(AggregateExpr::Type::SUM, nullptr);
    vector<Expression *> aggregate_exprs;
    aggregate_exprs.push_back(&aggregate_expr);
    standard_hash_table_ = make_unique<StandardAggregateHashTable>(aggregate_exprs);
  }

protected:
  unique_ptr<AggregateHashTable> standard_hash_table_;
};

BENCHMARK_DEFINE_F(DISABLED_StandardAggregateHashTableBenchmark, Aggregate)(benchmark::State &state)
{
  for (auto _ : state) {
    standard_hash_table_->add_chunk(group_chunk_, aggr_chunk_);
  }
}

BENCHMARK_REGISTER_F(DISABLED_StandardAggregateHashTableBenchmark, Aggregate)->Arg(16)->Arg(1024)->Arg(8192);

#ifdef USE_SIMD
class DISABLED_LinearProbingAggregateHashTableBenchmark : public AggregateHashTableBenchmark
{
public:
  void SetUp(const ::benchmark::State &state) override
  {

    AggregateHashTableBenchmark::SetUp(state);
    linear_probing_hash_table_ = make_unique<LinearProbingAggregateHashTable<int>>(AggregateExpr::Type::SUM);
  }

protected:
  unique_ptr<AggregateHashTable> linear_probing_hash_table_;
};

BENCHMARK_DEFINE_F(DISABLED_LinearProbingAggregateHashTableBenchmark, Aggregate)(benchmark::State &state)
{
  for (auto _ : state) {
    linear_probing_hash_table_->add_chunk(group_chunk_, aggr_chunk_);
  }
}

BENCHMARK_REGISTER_F(DISABLED_LinearProbingAggregateHashTableBenchmark, Aggregate)->Arg(16)->Arg(1024)->Arg(8192);
#endif

BENCHMARK_MAIN();