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
#include <atomic>
#include <thread>

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

using malloc_func_t = void *(*)(size_t);
using free_func_t   = void (*)(void *);
using mmap_func_t   = void *(*)(void *, size_t, int, int, int, off_t);
using munmap_func_t = int (*)(void *, size_t);

extern malloc_func_t orig_malloc;
extern free_func_t   orig_free;
extern mmap_func_t   orig_mmap;
extern munmap_func_t orig_munmap;

#define MT MemTracer::get_instance()

class MemTracer
{
public:
  inline static MemTracer &get_instance()
  {
    static MemTracer instance;
    return instance;
  }

  MemTracer();

  static void __attribute__((constructor)) init();

  static void __attribute__((destructor)) destroy();

  inline void init_hook_funcs() { pthread_once(&init_hook_funcs_once_, init_hook_funcs_impl); }

  void alloc(size_t size);

  void free(size_t size);

  inline size_t allocated_memory() const { return allocated_memory_.load(); }

  inline size_t meta_memory() const { return ((alloc_cnt_.load() - free_cnt_.load()) * sizeof(size_t)); }

  void set_memory_limit(size_t memory_limit) { memory_limit_ = memory_limit; }

  void set_print_interval(size_t print_interval) { print_interval_ = print_interval; }

  inline size_t print_interval() { return print_interval_; }

  inline size_t memory_limit() const { return memory_limit_; }

  inline void add_allocated_memory(size_t size) { allocated_memory_.fetch_add(size); }

  void stop();

  inline bool is_stop() { return is_stop_; }

private:
  static void init_hook_funcs_impl();

  void init_stats_thread();

  static void stat();

private:
  bool                is_inited_;
  bool                is_stop_;
  std::atomic<size_t> allocated_memory_;
  std::atomic<size_t> alloc_cnt_;
  std::atomic<size_t> free_cnt_;
  pthread_once_t      init_hook_funcs_once_;
  size_t              memory_limit_;
  size_t              print_interval_;
  std::thread         t_;
};