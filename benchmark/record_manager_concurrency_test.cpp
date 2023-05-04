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
// Created by Wangyunlai on 2023/05/04
//

#include <inttypes.h>
#include <random>
#include <stdexcept>
#include <benchmark/benchmark.h>

#include "storage/record/record_manager.h"
#include "storage/default/disk_buffer_pool.h"
#include "rc.h"
#include "common/log/log.h"

using namespace std;
using namespace common;
using namespace benchmark;

once_flag init_bpm_flag;
BufferPoolManager bpm{512};

struct Stat
{
  int64_t insert_success_count = 0;
  int64_t duplicate_count = 0;
  int64_t insert_other_count = 0;

  int64_t delete_success_count = 0;
  int64_t not_exist_count = 0;
  int64_t delete_other_count = 0;

  int64_t scan_success_count = 0;
  int64_t scan_open_failed_count = 0;
  int64_t mismatch_count = 0;
  int64_t scan_other_count = 0;
};

class BenchmarkBase : public Fixture
{
public:
  BenchmarkBase()
  {
  }

  virtual ~BenchmarkBase()
  {
    BufferPoolManager::set_instance(nullptr);
  }

  virtual string Name() const = 0;

  virtual void SetUp(const State &state)
  {
    if (0 != state.thread_index()) {
      return;
    }

    string log_name = this->Name() + ".log";
    string record_filename = this->Name() + ".record";
    LoggerFactory::init_default(log_name.c_str(), LOG_LEVEL_TRACE);

    std::call_once(init_bpm_flag, []() { BufferPoolManager::set_instance(&bpm); });

    ::remove(record_filename.c_str());

    RC rc = bmp.create_file(record_filename.c_str());
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to create record buffer pool file. filename=%s, rc=%s",
               record_filename.c_str(), strrc(rc));
      throw runtime_error("failed to create record buffer pool file.");
    }

    rc = bmp.open_file
    RC rc = handler_.init(bmp)
    if (rc != RC::SUCCESS) {
      throw runtime_error("failed to create btree handler");
    }
    LOG_INFO("test %s setup done. threads=%d, thread index=%d", 
             this->Name().c_str(), state.threads(), state.thread_index());
  }

  virtual void TearDown(const State &state)
  {
    if (0 != state.thread_index()) {
      return;
    }

    handler_.close();
    LOG_INFO("test %s teardown done. threads=%d, thread index=%d", 
             this->Name().c_str(), state.threads(), state.thread_index());
  }

  void FillUp(uint32_t min, uint32_t max)
  {
    for (uint32_t value = min; value < max; ++value) {
      const char *key = reinterpret_cast<const char *>(&value);
      RID rid(value, value);
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
    RID rid(value, value);
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
    RID rid(value, value);
    RC rc = handler_.delete_entry(key, &rid);
    switch (rc) {
      case RC::SUCCESS: {
        stat.delete_success_count++;
      } break;
      case RC::RECORD_RECORD_NOT_EXIST: {
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
    const char *end_key = reinterpret_cast<const char *>(&end);

    BplusTreeScanner scanner(handler_);
    RC rc = scanner.open(begin_key, sizeof(begin_key), true /*inclusive*/,
                         end_key, sizeof(end_key), true /*inclusive*/);
    if (rc != RC::SUCCESS) {
      stat.scan_open_failed_count++;
    } else {
      RID rid;
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
  DiskBufferPool    * buffer_pool_ = nullptr;
  RecordFileHandler handler_;
};
