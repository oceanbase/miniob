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
// Created by Wangyunlai on 2023/01/11.
//

#pragma once

#include <stdint.h>
#include <map>
#include <memory>
#include <mutex>
#include <atomic>
#include <chrono>
#include <thread>

#include "common/queue/queue.h"
#include "common/thread/runnable.h"

namespace common {

/**
 * @brief 模拟java ThreadPoolExecutor 做一个简化的线程池
 * @defgroup ThreadPool
 * @details 一个线程池包含一个任务队列和一组线程，当有任务提交时，线程池会从任务队列中取出任务分配给一个线程执行。
 * 这里的接口设计参考了Java的线程池ThreadPoolExecutor，但是简化了很多。
 * 
 * 这个线程池支持自动伸缩。
 * 线程分为两类，一类是核心线程，一类是普通线程。核心线程不会退出，普通线程会在空闲一段时间后退出。
 * 线程池有一个任务队列，收到的任务会放到任务队列中。当任务队列中任务的个数比当前线程个数多时，就会
 * 创建新的线程。
 */
class ThreadPoolExecutor
{
public:
  ThreadPoolExecutor() = default;
  virtual ~ThreadPoolExecutor();

  /**
   * @brief 初始化线程池
   * 
   * @param name 线程池名称
   * @param core_size 
   * @param max_size 
   * @param keep_alive_time_ms 
   * @return int 
   */
  int init(const char *name,
          int core_size,
          int max_size,
          long keep_alive_time_ms);

  int init(const char *name,
           int core_pool_size,
           int max_pool_size,
           long keep_alive_time_ms,
           std::unique_ptr<Queue<std::unique_ptr<Runnable>>> &&work_queue);

  int execute(std::unique_ptr<Runnable> &&task);
  int execute(const std::function<void()> &callable);
  int shutdown();
  int await_termination();

public:
  int active_count() const { return active_count_.load(); }
  int core_pool_size() const { return core_pool_size_; }
  int pool_size() const { return static_cast<int>(threads_.size()); }
  int largest_pool_size() const { return largest_pool_size_; }
  int64_t task_count() const { return task_count_.load();}

private:
  int create_thread(bool core_thread);
  int create_thread_locked(bool core_thread);
  int extend_thread();

private:
  void thread_func();
  
private:
  enum class State
  {
    NEW,
    RUNNING,
    TERMINATING,
    TERMINATED
  };

  struct ThreadState
  {
    bool core_thread = false;
    bool idle = false;
    bool terminated = false;
    std::thread *thread_ptr = nullptr;
  };

private:
  State state_ = State::NEW;

  int core_pool_size_ = 0;
  int max_pool_size_  = 0;
  std::chrono::milliseconds keep_alive_time_ms_;
  std::unique_ptr<Queue<std::unique_ptr<Runnable>>> work_queue_;

  mutable std::mutex lock_;
  std::map<std::thread::id, ThreadState> threads_;
  int largest_pool_size_ = 0; /// 历史上达到的最大的线程个数

  std::atomic<int64_t> task_count_ = 0;
  std::atomic<int>     active_count_ = 0; /// 活跃线程个数

  std::string pool_name_;
};

} // namespace common
