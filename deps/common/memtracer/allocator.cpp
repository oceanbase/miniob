/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "common/memtracer/allocator.h"
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>  // mmap/munmap

// `dlsym` calls `calloc` internally, so here use a dummy buffer
// to avoid infinite loop when hook functions initialized.
// ref:
// https://stackoverflow.com/questions/7910666/problems-with-ld-preload-and-calloc-interposition-for-certain-executables
static unsigned char calloc_buffer[8192];

extern "C" void *malloc(size_t size)
{
  MT.init_hook_funcs();
  size_t *ptr = (size_t *)orig_malloc(size + sizeof(size_t));
  if (unlikely(ptr == NULL)) {
    return NULL;
  }
  *ptr = size;
  MT.alloc(size);
  return (void *)(ptr + 1);
}

extern "C" void *calloc(size_t nelem, size_t size)
{
  if (unlikely(orig_malloc == NULL)) {
    return calloc_buffer;
  }
  MT.init_hook_funcs();
  size_t alloc_size = nelem * size;
  void  *ptr        = malloc(alloc_size);
  if (unlikely(ptr == NULL)) {
    return NULL;
  }
  memset(ptr, 0, alloc_size);
  return ptr;
}

inline size_t ptr_size(void *ptr)
{
  if (unlikely(ptr == NULL)) {
    return 0;
  }
  return *((size_t *)ptr - 1);
}

extern "C" void *realloc(void *ptr, size_t size)
{
  MT.init_hook_funcs();
  if (ptr == NULL) {
    return malloc(size);
  }
  void *res = NULL;
  if (ptr_size(ptr) < size) {
    res = malloc(size);
    if (unlikely(res == NULL)) {
      return NULL;
    }
    memcpy(res, ptr, ptr_size(ptr));

    free(ptr);
  } else {
    res = ptr;
  }
  return res;
}

extern "C" void free(void *ptr)
{
  MT.init_hook_funcs();
  if (unlikely(ptr == NULL || ptr == calloc_buffer)) {
    return;
  }
  MT.free(ptr_size(ptr));
  orig_free((size_t *)ptr - 1);
}

extern "C" void *memalign(size_t alignment, size_t size)
{
  fprintf(stderr, "memalign not supported\n");
  exit(-1);
}

extern "C" void *valloc(size_t size)
{
  fprintf(stderr, "valloc not supported\n");
  exit(-1);
}
extern "C" int posix_memalign(void **memptr, size_t alignment, size_t size)
{
  fprintf(stderr, "posix_memalign not supported\n");
  exit(-1);
}

extern "C" int brk(void *addr)
{
  fprintf(stderr, "brk not supported\n");
  exit(-1);
}

extern "C" void *sbrk(intptr_t increment)
{
  fprintf(stderr, "sbrk not supported\n");
  exit(-1);
}

extern "C" void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
  MT.init_hook_funcs();
  void *res = orig_mmap(addr, length, prot, flags, fd, offset);
  if (likely(res != MAP_FAILED)) {
    MT.alloc(length);
  }
  return res;
}

extern "C" int munmap(void *addr, size_t length)
{
  MT.init_hook_funcs();
  int res = orig_munmap(addr, length);
  if (likely(res == 0)) {
    MT.free(length);
  }
  return res;
}

extern "C" long int syscall(long int __sysno, ...)
{
  fprintf(stderr, "syscall not supported\n");
  exit(-1);
}

void *operator new(std::size_t size) { return malloc(size); }

void *operator new[](std::size_t size) { return malloc(size); }

void *operator new(std::size_t size, const std::nothrow_t &) noexcept { return malloc(size); }

void *operator new[](std::size_t size, const std::nothrow_t &) noexcept { return malloc(size); }

void operator delete(void *ptr) noexcept { free(ptr); }

void operator delete[](void *ptr) noexcept { free(ptr); }

void operator delete(void *ptr, const std::nothrow_t &) noexcept { free(ptr); }

void operator delete[](void *ptr, const std::nothrow_t &) noexcept { free(ptr); }

void operator delete(void *ptr, std::size_t size) noexcept { free(ptr); }
void operator delete[](void *ptr, std::size_t size) noexcept { free(ptr); }

extern "C" char *strdup(const char *__s)
{
  size_t len = strlen(__s);
  char  *p   = (char *)malloc(len + 1);
  if (p == NULL) {
    return NULL;
  }
  memcpy(p, __s, len);
  p[len] = 0;
  return p;
}
#ifdef __linux__
extern "C" void *__libc_malloc(size_t size) __attribute__((alias("malloc"), used));
extern "C" void *__libc_calloc(size_t nmemb, size_t size) __attribute__((alias("calloc"), used));
extern "C" void *__libc_realloc(void *ptr, size_t size) __attribute__((alias("realloc"), used));
extern "C" void  __libc_free(void *ptr) __attribute__((alias("free"), used));
extern "C" void *__libc_valloc(size_t size) __attribute__((alias("valloc"), used));
extern "C" void *__libc_memalign(size_t alignment, size_t size) __attribute__((alias("memalign"), used));
extern "C" int   __libc_posix_memalign(void **memptr, size_t alignment, size_t size)
    __attribute__((alias("posix_memalign"), used));
extern "C" int   __libc_brk(void *end_data_segment) __attribute__((alias("brk"), used));
extern "C" void *__libc_sbrk(ptrdiff_t increment) __attribute__((alias("sbrk"), used));
extern "C" void *__libc_mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset)
    __attribute__((alias("mmap"), used));
extern "C" long int __libc_syscall(long int number, ...) __attribute__((alias("syscall"), used));
extern "C" char *   __libc_strdup(const char *__s) __attribute__((alias("strdup"), used));
#endif