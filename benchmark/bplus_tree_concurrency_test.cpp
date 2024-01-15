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
// Created by Wangyunlai on 2023/03/14
//
#include <benchmark/benchmark.h>
#include <inttypes.h>
#include <stdexcept>

#include "common/log/log.h"
#include "common/math/integer_generator.h"
#include "storage/buffer/disk_buffer_pool.h"
#include "storage/index/bplus_tree.h"

using namespace std;
using namespace common;
using namespace benchmark;

once_flag         init_bpm_flag;
BufferPoolManager bpm{512};

struct Stat
{
  int64_t insert_success_count = 0;
  int64_t duplicate_count      = 0;
  int64_t insert_other_count   = 0;

  int64_t delete_success_count = 0;
  int64_t not_exist_count      = 0;
  int64_t delete_other_count   = 0;

  int64_t scan_success_count     = 0;
  int64_t scan_open_failed_count = 0;
  int64_t mismatch_count         = 0;
  int64_t scan_other_count       = 0;
};

class BenchmarkBase : public Fixture
{
public:
  BenchmarkBase() {}

  virtual ~BenchmarkBase() { BufferPoolManager::set_instance(nullptr); }

  virtual string Name() const = 0;

  virtual void SetUp(const State &state)
  {
    if (0 != state.thread_index()) {
      return;
    }

    string log_name       = this->Name() + ".log";
    string btree_filename = this->Name() + ".btree";
    LoggerFactory::init_default(log_name.c_str(), LOG_LEVEL_TRACE);

    std::call_once(init_bpm_flag, []() { BufferPoolManager::set_instance(&bpm); });

    ::remove(btree_filename.c_str());

    const int internal_max_size = 200;
    const int leaf_max_size     = 200;

    const char *filename = btree_filename.c_str();

    RC rc = handler_.create(filename, INTS, sizeof(int32_t) /*attr_len*/, internal_max_size, leaf_max_size);
    if (rc != RC::SUCCESS) {
      throw runtime_error("failed to create btree handler");
    }
    LOG_INFO(
        "test %s setup done. threads=%d, thread index=%d", this->Name().c_str(), state.threads(), state.thread_index());
  }

  virtual void TearDown(const State &state)
  {
    if (0 != state.thread_index()) {
      return;
    }

    handler_.close();
    LOG_INFO("test %s teardown done. threads=%d, thread index=%d",
        this->Name().c_str(),
        state.threads(),
        state.thread_index());
  }

  void FillUp(uint32_t min, uint32_t max)
  {
    for (uint32_t value = min; value < max; ++value) {
      const char *key = reinterpret_cast<const char *>(&value);
      RID         rid(value, value);

      [[maybe_unused]] RC rc = handler_.insert_entry(key, &rid);
      ASSERT(rc == RC::SUCCESS, "failed to insert entry into btree. key=%" PRIu32, value);
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

  void Insert(uint32_t value, Stat &stat)
  {
    const char *key = reinterpret_cast<const char *>(&value);
    RID         rid(value, value);

    RC rc = handler_.insert_entry(key, &rid);
    switch (rc) {
      case RC::SUCCESS: {
        stat.insert_success_count++;
      } break;
      case RC::RECORD_DUPLICATE_KEY: {
        stat.duplicate_count++;
      } break;
      default: {
        stat.insert_other_count++;
      } break;
    }
  }

  void Delete(uint32_t value, Stat &stat)
  {
    const char *key = reinterpret_cast<const char *>(&value);
    RID         rid(value, value);

    RC rc = handler_.delete_entry(key, &rid);
    switch (rc) {
      case RC::SUCCESS: {
        stat.delete_success_count++;
      } break;
      case RC::RECORD_NOT_EXIST: {
        stat.not_exist_count++;
      } break;
      default: {
        stat.delete_other_count++;
      } break;
    }
  }

  void Scan(uint32_t begin, uint32_t end, Stat &stat)
  {
    const char *begin_key = reinterpret_cast<const char *>(&begin);
    const char *end_key   = reinterpret_cast<const char *>(&end);

    BplusTreeScanner scanner(handler_);

    RC rc =
        scanner.open(begin_key, sizeof(begin_key), true /*inclusive*/, end_key, sizeof(end_key), true /*inclusive*/);
    if (rc != RC::SUCCESS) {
      stat.scan_open_failed_count++;
    } else {
      RID      rid;
      uint32_t count = 0;
      while (RC::RECORD_EOF != (rc = scanner.next_entry(rid))) {
        count++;
      }

      if (rc != RC::RECORD_EOF) {
        stat.scan_other_count++;
      } else if (count != (end - begin + 1)) {
        stat.mismatch_count++;
      } else {
        stat.scan_success_count++;
      }

      scanner.close();
    }
  }

protected:
  BplusTreeHandler handler_;
};

////////////////////////////////////////////////////////////////////////////////

struct InsertionBenchmark : public BenchmarkBase
{
  string Name() const override { return "insertion"; }
};

BENCHMARK_DEFINE_F(InsertionBenchmark, Insertion)(State &state)
{
  IntegerGenerator generator(1, 1 << 31);
  Stat             stat;

  for (auto _ : state) {
    uint32_t value = static_cast<uint32_t>(generator.next());
    Insert(value, stat);
  }

  state.counters["success"]   = Counter(stat.insert_success_count, Counter::kIsRate);
  state.counters["duplicate"] = Counter(stat.duplicate_count, Counter::kIsRate);
  state.counters["other"]     = Counter(stat.insert_other_count, Counter::kIsRate);
}

BENCHMARK_REGISTER_F(InsertionBenchmark, Insertion)->Threads(10);

////////////////////////////////////////////////////////////////////////////////

class DeletionBenchmark : public BenchmarkBase
{
public:
  string Name() const override { return "deletion"; }

  void SetUp(const State &state) override
  {
    if (0 != state.thread_index()) {
      return;
    }

    BenchmarkBase::SetUp(state);

    uint32_t max = GetRangeMax(state);
    ASSERT(max > 0, "invalid argument count. %ld", state.range(0));
    FillUp(0, max);
  }
};

BENCHMARK_DEFINE_F(DeletionBenchmark, Deletion)(State &state)
{
  uint32_t         max = GetRangeMax(state);
  IntegerGenerator generator(0, max);
  Stat             stat;

  for (auto _ : state) {
    uint32_t value = static_cast<uint32_t>(generator.next());
    Delete(value, stat);
  }

  state.counters["success"]   = Counter(stat.delete_success_count, Counter::kIsRate);
  state.counters["not_exist"] = Counter(stat.not_exist_count, Counter::kIsRate);
  state.counters["other"]     = Counter(stat.delete_other_count, Counter::kIsRate);
}

BENCHMARK_REGISTER_F(DeletionBenchmark, Deletion)->Threads(10)->Arg(4 * 10000);

////////////////////////////////////////////////////////////////////////////////

class ScanBenchmark : public BenchmarkBase
{
public:
  string Name() const override { return "scan"; }

  void SetUp(const State &state) override
  {
    if (0 != state.thread_index()) {
      return;
    }

    BenchmarkBase::SetUp(state);

    uint32_t max = static_cast<uint32_t>(state.range(0)) * 3;
    ASSERT(max > 0, "invalid argument count. %ld", state.range(0));
    FillUp(0, max);
  }
};

BENCHMARK_DEFINE_F(ScanBenchmark, Scan)(State &state)
{
  int              max_range_size = 100;
  uint32_t         max            = GetRangeMax(state);
  IntegerGenerator begin_generator(1, max - max_range_size);
  IntegerGenerator range_generator(1, max_range_size);
  Stat             stat;

  for (auto _ : state) {
    uint32_t begin = static_cast<uint32_t>(begin_generator.next());
    uint32_t end   = begin + static_cast<uint32_t>(range_generator.next());
    Scan(begin, end, stat);
  }

  state.counters["success"]               = Counter(stat.scan_success_count, Counter::kIsRate);
  state.counters["open_failed_count"]     = Counter(stat.scan_open_failed_count, Counter::kIsRate);
  state.counters["mismatch_number_count"] = Counter(stat.mismatch_count, Counter::kIsRate);
  state.counters["other"]                 = Counter(stat.scan_other_count, Counter::kIsRate);
}

BENCHMARK_REGISTER_F(ScanBenchmark, Scan)->Threads(10)->Arg(4 * 10000);

////////////////////////////////////////////////////////////////////////////////

struct MixtureBenchmark : public BenchmarkBase
{
  string Name() const override { return "mixture"; }
};

BENCHMARK_DEFINE_F(MixtureBenchmark, Mixture)(State &state)
{
  pair<uint32_t, uint32_t> data_range{0, GetRangeMax(state)};
  pair<uint32_t, uint32_t> scan_range{1, 100};

  IntegerGenerator data_generator(data_range.first, data_range.second);
  IntegerGenerator scan_range_generator(scan_range.first, scan_range.second);
  IntegerGenerator operation_generator(0, 2);

  Stat stat;

  for (auto _ : state) {
    int64_t operation_type = operation_generator.next();
    switch (operation_type) {
      case 0: {  // insert
        uint32_t value = static_cast<uint32_t>(data_generator.next());
        Insert(value, stat);
      } break;
      case 1: {  // delete
        uint32_t value = static_cast<uint32_t>(data_generator.next());
        Delete(value, stat);
      } break;
      case 2: {  // scan
        uint32_t begin = static_cast<uint32_t>(data_generator.next());
        uint32_t end   = begin + static_cast<uint32_t>(scan_range_generator.next());
        Scan(begin, end, stat);
      } break;
      default: {
        ASSERT(false, "should not happen. operation=%ld", operation_type);
      }
    }
  }

  state.counters.insert({{"insert_success", Counter(stat.insert_success_count, Counter::kIsRate)},
      {"insert_other", Counter(stat.insert_other_count, Counter::kIsRate)},
      {"insert_duplicate", Counter(stat.duplicate_count, Counter::kIsRate)},
      {"delete_success", Counter(stat.delete_success_count, Counter::kIsRate)},
      {"delete_other", Counter(stat.delete_other_count, Counter::kIsRate)},
      {"delete_not_exist", Counter(stat.not_exist_count, Counter::kIsRate)},
      {"scan_success", Counter(stat.scan_success_count, Counter::kIsRate)},
      {"scan_other", Counter(stat.scan_other_count, Counter::kIsRate)},
      {"scan_mismatch", Counter(stat.mismatch_count, Counter::kIsRate)},
      {"scan_open_failed", Counter(stat.scan_open_failed_count, Counter::kIsRate)}});
}

BENCHMARK_REGISTER_F(MixtureBenchmark, Mixture)->Threads(10)->Arg(4 * 10000);

////////////////////////////////////////////////////////////////////////////////

BENCHMARK_MAIN();
