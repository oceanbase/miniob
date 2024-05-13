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
#include <execinfo.h>
#include <iostream>
#include <cstdio>

#define REACH_TIME_INTERVAL(i)                                                                                \
  ({                                                                                                          \
    bool                    bret      = false;                                                                \
    static volatile int64_t last_time = 0;                                                                    \
    auto                    now       = std::chrono::system_clock::now();                                     \
    int64_t cur_time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count(); \
    if (int64_t(i + last_time) < cur_time) [[unlikely]] {                                                     \
      last_time = cur_time;                                                                                   \
      bret      = true;                                                                                       \
    }                                                                                                         \
    bret;                                                                                                     \
  })
extern memtracer::malloc_func_t orig_malloc;
extern memtracer::free_func_t   orig_free;
extern memtracer::mmap_func_t   orig_mmap;
extern memtracer::munmap_func_t orig_munmap;

namespace memtracer {
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

MemTracer &MemTracer::get_instance()
{
  static MemTracer instance;
  return instance;
}

void MemTracer::init()
{
  MT.init_hook_funcs();

  // init memory limit
  const char *memory_limit_str = std::getenv("MT_MEMORY_LIMIT");
  if (memory_limit_str != nullptr) {
    char *             end;
    unsigned long long value = std::strtoull(memory_limit_str, &end, 10);
    if (end != memory_limit_str && *end == '\0') {
      MT.set_memory_limit(static_cast<size_t>(value));
    } else {
      MEMTRACER_LOG("Invalid environment variable value for MT_MEMORY_LIMIT: %s\n", memory_limit_str);
    }
  }

  // init print_interval
  const char *print_interval_ms_str = std::getenv("MT_PRINT_INTERVAL_MS");
  if (print_interval_ms_str != nullptr) {
    char *             end;
    unsigned long long value = std::strtoull(print_interval_ms_str, &end, 10);
    if (end != print_interval_ms_str && *end == '\0') {
      MT.set_print_interval(static_cast<size_t>(value));
    } else {
      MEMTRACER_LOG("Invalid environment variable value for MT_MEMORY_LIMIT: %s\n", print_interval_ms_str);
    }
  } else {
    MT.set_print_interval(1000 * 5);  // 5s
  }

  // init `text` memory usage
  // TODO: support static variables statistic.
  size_t        text_size = 0;
  std::ifstream file("/proc/self/status");
  if (file.is_open()) {
    std::string line;
    const int   KB = 1024;
    while (std::getline(file, line)) {
      if (line.find("VmExe") != std::string::npos) {
        text_size = get_memory_size(line) * KB;
        break;
      }
    }
    file.close();
  }

  MT.add_allocated_memory(text_size);
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
  if (allocated_memory_.fetch_add(size) + size > memory_limit_) [[unlikely]] {
    MEMTRACER_LOG("alloc memory:%lu, allocated_memory: %lu, memory_limit: %lu, Memory limit exceeded!\n",
        size,
        allocated_memory_.load(),
        memory_limit_);
    exit(-1);
  }
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
  const size_t print_interval_ms = MT.print_interval();
  const size_t sleep_interval = 100;  // 100 ms
  while (!MT.is_stop()) {
    if (REACH_TIME_INTERVAL(print_interval_ms)) {
      // TODO: optimize the output format
      MEMTRACER_LOG("allocated memory: %lu, metadata memory: %lu\n", MT.allocated_memory(), MT.meta_memory());
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_interval));
  }
}
}  // namespace memtracer