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

static void BM_MallocFree(benchmark::State &state)
{
  size_t size = state.range(0);
  for (auto _ : state) {
    void *ptr = malloc(size);
    benchmark::DoNotOptimize(ptr);
    free(ptr);
  }
  state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * size));
}

static void BM_NewDelete(benchmark::State &state)
{
  size_t size = state.range(0);
  for (auto _ : state) {
    char *ptr = new char[size];
    benchmark::DoNotOptimize(ptr);
    delete[] ptr;
  }
  state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * size));
}

BENCHMARK(BM_MallocFree)->Arg(8)->Arg(64)->Arg(512)->Arg(1 << 10)->Arg(1 << 20)->Arg(8 << 20)->Arg(1 << 30);
BENCHMARK(BM_NewDelete)->Arg(8)->Arg(64)->Arg(512)->Arg(1 << 10)->Arg(1 << 20)->Arg(8 << 20)->Arg(1 << 30);

BENCHMARK_MAIN();