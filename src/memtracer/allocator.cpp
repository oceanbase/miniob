/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include <sys/mman.h>  // mmap/munmap
#include <string.h>

#include "memtracer/allocator.h"

// `dlsym` calls `calloc` internally, so here use a dummy buffer
// to avoid infinite loop when hook functions initialized.
// ref:
// https://stackoverflow.com/questions/7910666/problems-with-ld-preload-and-calloc-interposition-for-certain-executables
static unsigned char calloc_buffer[8192];

// only used internally
inline size_t ptr_size(void *ptr)
{
  if (ptr == NULL) [[unlikely]] {
    return 0;
  }
  return *((size_t *)ptr - 1);
}

mt_visible void *malloc(size_t size)
{
  MT.init_hook_funcs();
  size_t *ptr = (size_t *)orig_malloc(size + sizeof(size_t));
  if (ptr == NULL) [[unlikely]] {
    return NULL;
  }
  *ptr = size;
  MT.alloc(size);
  return (void *)(ptr + 1);
}

mt_visible void *calloc(size_t nelem, size_t size)
{
  if (orig_malloc == NULL) [[unlikely]] {
    return calloc_buffer;
  }
  size_t alloc_size = nelem * size;
  void * ptr        = malloc(alloc_size);
  if (ptr == NULL) [[unlikely]] {
    return NULL;
  }
  memset(ptr, 0, alloc_size);
  return ptr;
}

mt_visible void *realloc(void *ptr, size_t size)
{
  if (ptr == NULL) {
    return malloc(size);
  }
  void *res = NULL;
  if (ptr_size(ptr) < size) {
    res = malloc(size);
    if (res == NULL) [[unlikely]] {
      return NULL;
    }
    memcpy(res, ptr, ptr_size(ptr));

    free(ptr);
  } else {
    res = ptr;
  }
  return res;
}

mt_visible void free(void *ptr)
{
  MT.init_hook_funcs();
  if (ptr == NULL || ptr == calloc_buffer) [[unlikely]] {
    return;
  }
  MT.free(ptr_size(ptr));
  orig_free((size_t *)ptr - 1);
}

mt_visible void cfree(void *ptr) { free(ptr); }

mt_visible void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
  MT.init_hook_funcs();
  void *res = orig_mmap(addr, length, prot, flags, fd, offset);
  if (res != MAP_FAILED) [[likely]] {
    MT.alloc(length);
  }
  return res;
}

mt_visible int munmap(void *addr, size_t length)
{
  MT.init_hook_funcs();
  int res = orig_munmap(addr, length);
  if (res == 0) [[likely]] {
    MT.free(length);
  }
  return res;
}

mt_visible char *strdup(const char *s) MT_THROW
{
  size_t len = strlen(s);
  char * p   = (char *)malloc(len + 1);
  if (p == NULL) {
    return NULL;
  }
  memcpy(p, s, len);
  p[len] = 0;
  return p;
}

mt_visible char *strndup(const char *s, size_t n) MT_THROW
{
  const char * end = (const char *)memchr(s, 0, n);
  const size_t m   = (end != NULL ? (size_t)(end - s) : n);
  char *       t   = (char *)malloc(m + 1);
  if (t == NULL)
    return NULL;
  memcpy(t, s, m);
  t[m] = 0;
  return t;
}

mt_visible char *realpath(const char *fname, char *resolved_name)
{
  MEMTRACER_LOG("realpath not supported\n");
  exit(-1);
}
mt_visible void *memalign(size_t alignment, size_t size)
{
  MEMTRACER_LOG("memalign not supported\n");
  exit(-1);
}

mt_visible void *valloc(size_t size)
{
  MEMTRACER_LOG("valloc not supported\n");
  exit(-1);
}

mt_visible void *pvalloc(size_t size)
{
  MEMTRACER_LOG("valloc not supported\n");
  exit(-1);
}

mt_visible int posix_memalign(void **memptr, size_t alignment, size_t size)
{
  MEMTRACER_LOG("posix_memalign not supported\n");
  exit(-1);
}

#ifdef LINUX
mt_visible int brk(void *addr)
{
  MEMTRACER_LOG("brk not supported\n");
  exit(-1);
}

mt_visible void *sbrk(intptr_t increment)
{
  MEMTRACER_LOG("sbrk not supported\n");
  exit(-1);
}

mt_visible long int syscall(long int __sysno, ...)
{
  MEMTRACER_LOG("syscall not supported\n");
  exit(-1);
}
#elif defined(__MACH__)
mt_visible void    *brk(const void *addr)
{
  MEMTRACER_LOG("brk not supported\n");
  exit(-1);
}
mt_visible void *sbrk(int increment)
{
  MEMTRACER_LOG("sbrk not supported\n");
  exit(-1);
}
mt_visible int syscall(int __sysno, ...)
{
  MEMTRACER_LOG("syscall not supported\n");
  exit(-1);
}
#endif

mt_visible void *operator new(std::size_t size) { return malloc(size); }

mt_visible void *operator new[](std::size_t size) { return malloc(size); }

mt_visible void *operator new(std::size_t size, const std::nothrow_t &) noexcept { return malloc(size); }

mt_visible void *operator new[](std::size_t size, const std::nothrow_t &) noexcept { return malloc(size); }

mt_visible void operator delete(void *ptr) noexcept { free(ptr); }

mt_visible void operator delete[](void *ptr) noexcept { free(ptr); }

mt_visible void operator delete(void *ptr, const std::nothrow_t &) noexcept { free(ptr); }

mt_visible void operator delete[](void *ptr, const std::nothrow_t &) noexcept { free(ptr); }

mt_visible void operator delete(void *ptr, std::size_t size) noexcept { free(ptr); }

mt_visible void operator delete[](void *ptr, std::size_t size) noexcept { free(ptr); }