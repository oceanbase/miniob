/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include <cstdlib>
#include <sys/mman.h>  // mmap/munmap
#include <thread>
#include "gtest/gtest.h"
#include "memtracer/mt_info.h"

#ifdef __linux__
extern "C" void *__libc_malloc(size_t size);
#endif

class Foo
{
public:
  Foo() : val(1) {}
  int val;
};

void allocate_and_free_with_new_delete(size_t size)
{
  char *memory = new char[size];
  memset(memory, 0, size);
  delete[] memory;
}

void thread_function(void (*allocator)(size_t), size_t size) { allocator(size); }

void allocate_and_free_with_malloc_free(size_t size)
{
  char *memory = static_cast<char *>(malloc(size));
  memset(memory, 0, size);
  free(memory);
}

void perform_multi_threads_allocation(void (*allocator)(size_t), size_t size, size_t num_threads)
{
  std::vector<std::thread> threads;
  for (size_t i = 0; i < num_threads; ++i) {
    threads.emplace_back(thread_function, allocator, size);
  }
  for (auto &t : threads) {
    t.join();
  }
}

TEST(test_mem_tracer, test_mem_tracer_basic)
{
  if (getenv("LD_PRELOAD") == nullptr) {
    GTEST_SKIP();
  }
  size_t mem_base = memtracer::allocated_memory();
  // malloc/free
  {
    void *ptr = malloc(1024);
    memset(ptr, 0, 1024);
    // if no use the memory that allocate by malloc,
    // the compiler may optimize it out,
    // so we need to use it here (even memset is also optimized)
    *(char *)ptr = 'f';
    ASSERT_EQ(memtracer::allocated_memory(), mem_base + 1024);
    free(ptr);
    ASSERT_EQ(memtracer::allocated_memory(), mem_base);

    ptr = malloc(1024 * 1024 * 1024);
    memset(ptr, 0, 1024 * 1024 * 1024);
    *(char *)ptr = 'f';
    ASSERT_EQ(memtracer::allocated_memory(), mem_base + 1024 * 1024 * 1024);
    free(ptr);
    ASSERT_EQ(memtracer::allocated_memory(), mem_base);

    for (int i = 0; i < 1024; ++i) {
      ptr = malloc(1);
      memset(ptr, 0, 1);
      *(char *)ptr = 'f';
      ASSERT_EQ(memtracer::allocated_memory(), mem_base + 1);
      free(ptr);
      ASSERT_EQ(memtracer::allocated_memory(), mem_base);
    }
    ASSERT_EQ(memtracer::allocated_memory(), mem_base);
  }

  // new/delete
  {
    char *ptr = new char;
    *ptr      = 'a';
    ASSERT_EQ(memtracer::allocated_memory(), mem_base + 1);
    delete ptr;
    ASSERT_EQ(memtracer::allocated_memory(), mem_base);
  }

  // new/delete obj
  {
    Foo *ptr = new Foo;
    ASSERT_EQ(memtracer::allocated_memory(), mem_base + sizeof(Foo));
    ASSERT_EQ(1, ptr->val);
    delete ptr;
    ASSERT_EQ(memtracer::allocated_memory(), mem_base);
  }

  // new/delete array
  {
    char *ptr = new char[1024];
    memset(ptr, 0, 1024);
    *ptr = 'f';
    ASSERT_EQ(memtracer::allocated_memory(), mem_base + 1024);
    delete[] ptr;
    ASSERT_EQ(memtracer::allocated_memory(), mem_base);
  }

  // calloc/free
  {
    void *ptr = calloc(10, 100);
    memset(ptr, 0, 10 * 100);
    *(char *)ptr = 'a';
    ASSERT_EQ(memtracer::allocated_memory(), mem_base + 10 * 100);
    free(ptr);
    ASSERT_EQ(memtracer::allocated_memory(), mem_base);
  }

  // realloc/free
  {
    void *ptr = realloc(NULL, 10);
    memset(ptr, 0, 10);
    *(char *)ptr = 'f';
    ASSERT_EQ(memtracer::allocated_memory(), mem_base + 10);
    ptr = realloc(ptr, 1);
    memset(ptr, 0, 1);
    *(char *)ptr = 'f';
    ASSERT_EQ(memtracer::allocated_memory(), mem_base + 10);
    ptr = realloc(ptr, 100);
    memset(ptr, 0, 100);
    *(char *)ptr = 'f';
    ASSERT_EQ(memtracer::allocated_memory(), mem_base + 100);
    free(ptr);
    ASSERT_EQ(memtracer::allocated_memory(), mem_base);
  }
  // mmap/munmap
  {
    void *ptr    = mmap(nullptr, 1024, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    *(char *)ptr = 'f';
    ASSERT_NE(ptr, nullptr);
    ASSERT_EQ(memtracer::allocated_memory(), mem_base + 1024);
    munmap(ptr, 1024);
    ASSERT_EQ(memtracer::allocated_memory(), mem_base);
  }

// __libc_malloc
#ifdef __linux__
  {
    void *ptr = __libc_malloc(1024);
    memset(ptr, 0, 1024);
    *(char *)ptr = 'f';
    ASSERT_EQ(memtracer::allocated_memory(), mem_base + 1024);
    free(ptr);
    ASSERT_EQ(memtracer::allocated_memory(), mem_base);
  }
#endif

  // __builtin_malloc
  {
    void *ptr = __builtin_malloc(1024);
    memset(ptr, 0, 1024);
    *(char *)ptr = 'f';
    ASSERT_EQ(memtracer::allocated_memory(), mem_base + 1024);
    free(ptr);
    ASSERT_EQ(memtracer::allocated_memory(), mem_base);
  }
}

TEST(test_mem_tracer, test_mem_tracer_multi_threads)
{
  if (getenv("LD_PRELOAD") == nullptr) {
    GTEST_SKIP();
  }
  // Since some of the low-level functions also take up memory,
  // for example, pthread will alloc stack via `mmap` and
  // the thread's stack will not be released after thread join.
  // allocated memory count is not checked here.
  {
    size_t num_threads = 1;
    size_t memory_size = 16;

    perform_multi_threads_allocation(allocate_and_free_with_new_delete, memory_size, num_threads);
    perform_multi_threads_allocation(allocate_and_free_with_malloc_free, memory_size, num_threads);
  }
  {
    size_t num_threads = 4;
    size_t memory_size = 16;

    perform_multi_threads_allocation(allocate_and_free_with_new_delete, memory_size, num_threads);
    perform_multi_threads_allocation(allocate_and_free_with_malloc_free, memory_size, num_threads);
  }

  {
    size_t num_threads = 16;
    size_t memory_size = 1024;

    perform_multi_threads_allocation(allocate_and_free_with_new_delete, memory_size, num_threads);
    perform_multi_threads_allocation(allocate_and_free_with_malloc_free, memory_size, num_threads);
  }

  {
    size_t num_threads = 64;
    size_t memory_size = 1024;

    perform_multi_threads_allocation(allocate_and_free_with_new_delete, memory_size, num_threads);
    perform_multi_threads_allocation(allocate_and_free_with_malloc_free, memory_size, num_threads);
  }
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}