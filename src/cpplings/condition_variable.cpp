/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

// condition_variable（条件变量）是 C++11 中提供的一种多线程同步机制，
// 它允许一个或多个线程等待另一个线程发出通知，以便能够有效地进行线程同步。
// wait() 函数用于阻塞线程并等待唤醒，使用 std::unique_lock<std::mutex> 来等待(`void wait(unique_lock<mutex>& Lck);`)
// 如果条件变量当前不满足，线程将被阻塞，同时释放锁，使得其他线程可以继续执行。
// 当另一个线程调用 notify_one() 或 notify_all() 来通知条件变量时，被阻塞的线程将被唤醒，并再次尝试获取锁。
// wait() 函数返回时，锁会再次被持有。
// 详细用法可参考：https://en.cppreference.com/w/cpp/thread/condition_variable

#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
#include <cassert>

int                     count             = 0;
int                     expect_thread_num = 10;
std::mutex              m;
std::condition_variable cv;

// TODO: 每次调用会增加 count 的值，当count 的值达到 expect 的时候通知 waiter_thread
void add_count_and_notify()
{
  std::scoped_lock slk(m);
  count += 1;
}

void waiter_thread()
{
  // TODO: 等待 count 的值达到 expect_thread_num，然后打印 count 的值
  std::cout << "Printing count: " << count << std::endl;
  assert(count == expect_thread_num);
}

int main()
{
  int                      thread_num = expect_thread_num;
  std::vector<std::thread> threads;
  std::thread              waiter(waiter_thread);
  for (int i = 0; i < thread_num; ++i)
    threads.push_back(std::thread(add_count_and_notify));
  waiter.join();
  for (auto &th : threads)
    th.join();
  std::cout << "passed!" << std::endl;
  return 0;
}