/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "memtracer.h"
#include <dlfcn.h>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <chrono>

const int KB = 1024;

// used only for getting memory size from `/proc/self/status`
long get_memory_size(const std::string &line)
{
  std::string        token;
  std::istringstream iss(line);
  long               size;
  // skip token
  iss >> token;
  iss >> size;
  return size;  // KB
}

#define REACH_TIME_INTERVAL(i)                                                                                \
  ({                                                                                                          \
    bool                    bret      = false;                                                                \
    static volatile int64_t last_time = 0;                                                                    \
    auto                    now       = std::chrono::system_clock::now();                                     \
    int64_t cur_time = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count(); \
    if (unlikely(int64_t(i + last_time) < cur_time)) {                                                        \
      last_time = cur_time;                                                                                   \
      bret      = true;                                                                                       \
    }                                                                                                         \
    bret;                                                                                                     \
  })

MemTracer::MemTracer()
    : is_inited_(false),
      is_stop_(false),
      allocated_memory_(0),
      alloc_cnt_(0),
      free_cnt_(0),
      init_hook_funcs_once_(PTHREAD_ONCE_INIT),
      memory_limit_(UINT64_MAX),
      print_interval_(0),
      t_()
{}

void MemTracer::init()
{
  MT.init_hook_funcs();

  // init memory limit
  const char *memory_limit_str = std::getenv("MT_MEMORY_LIMIT");
  if (memory_limit_str != nullptr) {
    char              *end;
    unsigned long long value = std::strtoull(memory_limit_str, &end, 10);
    if (end != memory_limit_str && *end == '\0') {
      MT.set_memory_limit(static_cast<size_t>(value));
    } else {
      fprintf(stderr, "Invalid environment variable value for MT_MEMORY_LIMIT: %s\n", memory_limit_str);
    }
  }

  // init print_interval
  const char *print_interval_str = std::getenv("MT_PRINT_INTERVAL");
  if (print_interval_str != nullptr) {
    char              *end;
    unsigned long long value = std::strtoull(print_interval_str, &end, 10);
    if (end != print_interval_str && *end == '\0') {
      MT.set_print_interval(static_cast<size_t>(value));
    } else {
      fprintf(stderr, "Invalid environment variable value for MT_MEMORY_LIMIT: %s\n", print_interval_str);
    }
  } else {
    MT.set_print_interval(1000 * 1000);  // 1s
  }

  // init `text` and `data` memory usage
  std::ifstream status("/proc/self/status");
  std::string   line;

  long data_segment_kb = 0;
  long code_segment_kb = 0;

  while (std::getline(status, line)) {
    if (line.find("VmData:") != std::string::npos) {
      data_segment_kb = get_memory_size(line);
    } else if (line.find("VmExe:") != std::string::npos) {
      code_segment_kb = get_memory_size(line);
    }
  }
  MT.add_allocated_memory(data_segment_kb * KB + code_segment_kb * KB);
  MT.init_stats_thread();
}

void MemTracer::init_stats_thread()
{
  if (!t_.joinable()) {
    t_ = std::thread(stat);
  }
}

void MemTracer::alloc(size_t size)
{
  if (unlikely(allocated_memory_.load() + size > memory_limit_)) {
    fprintf(stderr, "Memory limit exceeded!\n");
    exit(-1);
  }
  allocated_memory_.fetch_add(size);
  alloc_cnt_.fetch_add(1);
}

void MemTracer::free(size_t size)
{
  allocated_memory_.fetch_sub(size);
  free_cnt_.fetch_add(1);
}

void MemTracer::destroy() { MT.stop(); }

void MemTracer::stop()
{
  is_stop_ = true;
  t_.join();
}

void MemTracer::init_hook_funcs_impl()
{
  orig_malloc = (void *(*)(size_t size))dlsym(RTLD_NEXT, "malloc");
  orig_free   = (void (*)(void *ptr))dlsym(RTLD_NEXT, "free");
  orig_mmap = (void *(*)(void *addr, size_t length, int prot, int flags, int fd, off_t offset))dlsym(RTLD_NEXT, "mmap");
  orig_munmap = (int (*)(void *addr, size_t length))dlsym(RTLD_NEXT, "munmap");
}

void MemTracer::stat()
{
  const size_t print_interval = MT.print_interval();
  const size_t sleep_interval = 100 * 1000;  // 100 ms
  while (!MT.is_stop()) {
    if (REACH_TIME_INTERVAL(print_interval)) {
      // TODO: optimize the output format
      fprintf(
          stderr, "[MemTracer] allocated memory: %lu, metadata memory: %lu\n", MT.allocated_memory(), MT.meta_memory());
    }
    std::this_thread::sleep_for(std::chrono::microseconds(sleep_interval));
  }
}