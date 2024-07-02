/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#pragma once

#include <cstdlib>

#include "memtracer/common.h"
#include "common/lang/thread.h"
#include "common/lang/atomic.h"
#include "common/lang/mutex.h"

namespace memtracer {

#define MT MemTracer::get_instance()

// MemTracer is used to monitor MiniOB memory usage;
// it is used to run and debug MiniOB under memory-constrained conditions.
// MemTracer statistics memory allocation in MiniOB processes by
// hooking memory alloc/free functions.
// More detail can be found at docs/src/game/miniob-memtracer.md
class MemTracer
{
public:
  static MemTracer &get_instance();

  MemTracer() = default;

  static void __attribute__((constructor)) init();

  static void __attribute__((destructor)) destroy();

  size_t allocated_memory() const { return allocated_memory_.load(); }

  size_t meta_memory() const { return ((alloc_cnt_.load() - free_cnt_.load()) * sizeof(size_t)); }

  size_t print_interval() const { return print_interval_ms_; }

  size_t memory_limit() const { return memory_limit_; }

  bool is_stop() const { return is_stop_; }

  void set_print_interval(size_t print_interval_ms) { print_interval_ms_ = print_interval_ms; }

  inline void add_allocated_memory(size_t size) { allocated_memory_.fetch_add(size); }

  void set_memory_limit(size_t memory_limit)
  {
    call_once(memory_limit_once_, [&]() { memory_limit_ = memory_limit; });
  }

  void alloc(size_t size);

  void free(size_t size);

  inline void init_hook_funcs() { call_once(init_hook_funcs_once_, init_hook_funcs_impl); }

private:
  static void init_hook_funcs_impl();

  void init_stats_thread();

  void stop();

  static void stat();

private:
  bool           is_stop_ = false;
  atomic<size_t> allocated_memory_{};
  atomic<size_t> alloc_cnt_{};
  atomic<size_t> free_cnt_{};
  once_flag      init_hook_funcs_once_;
  once_flag      memory_limit_once_;
  size_t         memory_limit_      = UINT64_MAX;
  size_t         print_interval_ms_ = 0;
  thread         t_;
};
}  // namespace memtracer