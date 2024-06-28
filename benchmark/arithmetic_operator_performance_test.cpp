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

#include "sql/expr/arithmetic_operator.hpp"

class DISABLED_ArithmeticBenchmark : public benchmark::Fixture
{
public:
  void SetUp(const ::benchmark::State &state) override
  {
    int size = state.range(0);
    left_    = (float *)malloc(size * sizeof(float));
    right_   = (float *)malloc(size * sizeof(float));
    result_  = (float *)malloc(size * sizeof(float));

    for (int i = 0; i < size; ++i) {
      left_[i]   = 1.0f;
      right_[i]  = 0.5f;
      result_[i] = 0.0f;
    }
  }

  void TearDown(const ::benchmark::State &state) override
  {
    free(left_);
    left_ = nullptr;
    free(right_);
    right_ = nullptr;
    free(result_);
    result_ = nullptr;
  }

protected:
  float *left_   = nullptr;
  float *right_  = nullptr;
  float *result_ = nullptr;
};

BENCHMARK_DEFINE_F(DISABLED_ArithmeticBenchmark, Add)(benchmark::State &state)
{
  for (auto _ : state) {
    binary_operator<false, false, float, AddOperator>(left_, right_, result_, state.range(0));
  }
}

BENCHMARK_REGISTER_F(DISABLED_ArithmeticBenchmark, Add)->Arg(10)->Arg(1000)->Arg(10000);

BENCHMARK_DEFINE_F(DISABLED_ArithmeticBenchmark, Sub)(benchmark::State &state)
{
  for (auto _ : state) {
    binary_operator<false, false, float, SubtractOperator>(left_, right_, result_, state.range(0));
  }
}

BENCHMARK_REGISTER_F(DISABLED_ArithmeticBenchmark, Sub)->Arg(10)->Arg(1000)->Arg(10000);

#ifdef USE_SIMD
static void DISABLED_benchmark_sum_simd(benchmark::State &state)
{
  int              size = state.range(0);
  std::vector<int> data(state.range(0), 1);
  for (auto _ : state) {
    mm256_sum_epi32(data.data(), size);
  }
}

BENCHMARK(DISABLED_benchmark_sum_simd)->RangeMultiplier(2)->Range(1 << 10, 1 << 12);
#endif

static int sum_scalar(const int *data, int size)
{
  int sum = 0;
  for (int i = 0; i < size; i++) {
    sum += data[i];
  }
  return sum;
}

static void DISABLED_benchmark_sum_scalar(benchmark::State &state)
{
  int              size = state.range(0);
  std::vector<int> data(size, 1);
  for (auto _ : state) {
    int res = sum_scalar(data.data(), size);
    benchmark::DoNotOptimize(res);
  }
}

BENCHMARK(DISABLED_benchmark_sum_scalar)->RangeMultiplier(2)->Range(1 << 10, 1 << 12);

BENCHMARK_MAIN();