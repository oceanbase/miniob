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
#include <inttypes.h>

#include "common/lang/stdexcept.h"
#include "common/lang/filesystem.h"
#include "common/log/log.h"
#include "common/math/integer_generator.h"
#include "oblsm/include/ob_lsm.h"

// TODO
// a simple benchmark to test oblsm concurrency put/get. more detail test can use `ob_lsm_bench` tool.
using namespace std;
using namespace common;
using namespace benchmark;
using namespace oceanbase;

class BenchmarkBase : public Fixture
{
public:
  BenchmarkBase() {}

  virtual ~BenchmarkBase() {}

  virtual string Name() const = 0;

  virtual void SetUp(const State &state)
  {
    if (0 != state.thread_index()) {
      return;
    }
    filesystem::remove_all("oblsm_benchmark");
    filesystem::create_directory("oblsm_benchmark");

    RC rc = ObLsm::open(ObLsmOptions(), "oblsm_benchmark", &oblsm_);
    if (rc != RC::SUCCESS) {
      throw runtime_error("failed to open oblsm");
    }
    LOG_INFO("test %s setup done. threads=%d, thread index=%d",
             this->Name().c_str(), state.threads(), state.thread_index());
  }

  virtual void TearDown(const State &state)
  {
    if (0 != state.thread_index()) {
      return;
    }
    delete oblsm_;
    LOG_INFO("test %s teardown done. threads=%d, thread index=%d",
        this->Name().c_str(),
        state.threads(),
        state.thread_index());
  }

  void FillUp(uint32_t min, uint32_t max)
  {
    for (uint32_t value = min; value < max; ++value) {
      string key = to_string(value);

      RC rc = oblsm_->put(key, key);
      if (rc != RC::SUCCESS) {
        exit(1);
      }
    }
  }

  uint32_t GetRangeMax(const State &state) const
  {
    uint32_t max = static_cast<uint32_t>(state.range(0) * 3);
    if (max <= 0) {
      max = (1 << 31);
    }
    return max;
  }

  void Insert(uint32_t value)
  {
    RC rc = oblsm_->put(to_string(value), to_string(value));
    if (rc != RC::SUCCESS) {
      exit(1);
    }
  }

  void Scan(uint32_t begin, uint32_t end)
  {
    auto iter = oblsm_->new_iterator(ObLsmReadOptions());
    iter->seek(to_string(begin));
    while (iter->valid() && iter->key() != to_string(end)) {
      iter->next();
    }
    delete iter;
  }

protected:
  oceanbase::ObLsm *oblsm_ = nullptr;
};

////////////////////////////////////////////////////////////////////////////////

struct DISABLED_MixtureBenchmark : public BenchmarkBase
{
  string Name() const override { return "mixture"; }
};

BENCHMARK_DEFINE_F(DISABLED_MixtureBenchmark, Mixture)(State &state)
{
  pair<uint32_t, uint32_t> insert_range{GetRangeMax(state) + 1, GetRangeMax(state) * 2};
  pair<uint32_t, uint32_t> scan_range{1, 100};
  pair<uint32_t, uint32_t> data_range{0, GetRangeMax(state) * 2};

  IntegerGenerator data_generator(data_range.first, data_range.second);
  IntegerGenerator insert_generator(insert_range.first, insert_range.second);
  IntegerGenerator scan_range_generator(scan_range.first, scan_range.second);
  IntegerGenerator operation_generator(0, 10);

  for (auto _ : state) {
    int64_t operation_type = operation_generator.next();
    if (operation_type <= 9) {
      operation_type = 0;
    } else {
      operation_type = 1;
    }
    switch (operation_type) {
      case 0: {  // insert
        uint32_t value = static_cast<uint32_t>(insert_generator.next());
        Insert(value);
      } break;
      case 1: {  // scan
        uint32_t begin = static_cast<uint32_t>(data_generator.next());
        uint32_t end   = begin + static_cast<uint32_t>(scan_range_generator.next());
        Scan(begin, end);
      } break;
      default: {
        ASSERT(false, "should not happen. operation=%ld", operation_type);
      }
    }
  }
}

BENCHMARK_REGISTER_F(DISABLED_MixtureBenchmark, Mixture)->Threads(10)->Arg(1)->Arg(1000)->Arg(10000);

////////////////////////////////////////////////////////////////////////////////

BENCHMARK_MAIN();
