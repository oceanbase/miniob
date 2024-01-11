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
 * 
 */
class ThreadPoolExecutor
{
public:
  ThreadPoolExecutor() = default;
  virtual ~ThreadPoolExecutor();

  int init(const char *name,
           int core_pool_size,
           int max_pool_size,
           long keep_alive_time_ms,
           std::unique_ptr<Queue<std::unique_ptr<Runnable>>> &&work_queue);

  int execute(std::unique_ptr<Runnable> &&task);
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
