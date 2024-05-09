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

#include <stdio.h>

namespace memtracer {
#define mt_visible __attribute__((visibility("default")))

using malloc_func_t = void *(*)(size_t);
using free_func_t   = void (*)(void *);
using mmap_func_t   = void *(*)(void *, size_t, int, int, int, off_t);
using munmap_func_t = int (*)(void *, size_t);

void log_stderr(const char *format, ...);

#define MEMTRACER_LOG(format, ...)     \
  do {                                 \
    fprintf(stderr, "[MEMTRACER] ");   \
    log_stderr(format, ##__VA_ARGS__); \
  } while (0)

}  // namespace memtracer