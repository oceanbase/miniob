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
// Created by Wangyunlai on 2024/01/11.
//

#include "gtest/gtest.h"
#include "common/lang/memory.h"
#include "common/lang/atomic.h"
#include "common/queue/queue.h"
#include "common/queue/simple_queue.h"
#include "common/thread/runnable.h"
#include "common/thread/thread_pool_executor.h"

using namespace common;

class TestRunnable : public Runnable
{
public:
  TestRunnable(atomic<int> &counter) : counter_(counter) {}

  virtual ~TestRunnable() = default;

  virtual void run() override { ++counter_; }

private:
  atomic<int> &counter_;
};

class RandomSleepRunnable : public Runnable
{
public:
  RandomSleepRunnable(int min_ms, int max_ms) : min_ms_(min_ms), max_ms_(max_ms) {}

  virtual ~RandomSleepRunnable() = default;

  virtual void run() override
  {
    int sleep_ms = min_ms_ + rand() % (max_ms_ - min_ms_);
    this_thread::sleep_for(chrono::milliseconds(sleep_ms));
  }

private:
  int min_ms_ = 0;
  int max_ms_ = 0;
};

void test(int core_size, int max_pool_size, int keep_alive_time_ms, int test_num, function<Runnable *()> task_factory)
{
  ThreadPoolExecutor executor;

  int ret = executor.init("test",
      core_size,           // core_size
      max_pool_size,       // max_size
      keep_alive_time_ms,  // keep_alive_time_ms
      make_unique<SimpleQueue<unique_ptr<Runnable>>>());
  EXPECT_EQ(0, ret);
  EXPECT_EQ(core_size, executor.pool_size());

  for (int i = 0; i < test_num; ++i) {
    ret = executor.execute(unique_ptr<Runnable>(task_factory()));
    EXPECT_EQ(0, ret);
  }

  executor.shutdown();
  executor.await_termination();
  EXPECT_EQ(executor.task_count(), test_num);
}

TEST(ThreadPoolExecutor, test1)
{
  atomic<int> counter(0);
  test(1, 1, 60 * 1000, 1000000, [&counter]() { return new TestRunnable(counter); });
}

TEST(ThreadPoolExecutor, test2)
{
  atomic<int> counter(0);
  test(2, 8, 60 * 1000, 1000000, [&counter]() { return new TestRunnable(counter); });
}

TEST(ThreadPoolExecutor, test3)
{
  test(2, 8, 60 * 1000, 1000, []() { return new RandomSleepRunnable(10, 100); });
}

int main(int argc, char **argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
