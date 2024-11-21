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

#include <thread>

#include "common/thread/thread_pool_executor.h"
#include "common/log/log.h"
#include "common/queue/simple_queue.h"
#include "common/thread/thread_util.h"

using namespace std;

namespace common {

int ThreadPoolExecutor::init(const char *name, int core_pool_size, int max_pool_size, long keep_alive_time_ms)
{
  unique_ptr<Queue<unique_ptr<Runnable>>> queue_ptr(new (nothrow) SimpleQueue<unique_ptr<Runnable>>());
  return init(name, core_pool_size, max_pool_size, keep_alive_time_ms, std::move(queue_ptr));
}

int ThreadPoolExecutor::init(const char *name, int core_pool_size, int max_pool_size, long keep_alive_time_ms,
    unique_ptr<Queue<unique_ptr<Runnable>>> &&work_queue)
{
  if (state_ != State::NEW) {
    LOG_ERROR("invalid state. state=%d", state_);
    return -1;
  }

  if (core_pool_size < 0 || max_pool_size <= 0 || core_pool_size > max_pool_size) {
    LOG_ERROR("invalid argument. core_pool_size=%d, max_pool_size=%d", core_pool_size, max_pool_size);
    return -1;
  }

  if (name != nullptr) {
    pool_name_ = name;
  }

  core_pool_size_     = core_pool_size;
  max_pool_size_      = max_pool_size;
  keep_alive_time_ms_ = chrono::milliseconds(keep_alive_time_ms);
  work_queue_         = std::move(work_queue);

  while (static_cast<int>(threads_.size()) < core_pool_size_) {
    if (create_thread(true /*core_thread*/) != 0) {
      LOG_ERROR("create thread failed");
      return -1;
    }
  }

  state_ = State::RUNNING;
  return 0;
}

ThreadPoolExecutor::~ThreadPoolExecutor()
{
  if (state_ != State::TERMINATED) {
    shutdown();
    await_termination();
  }
}

int ThreadPoolExecutor::shutdown()
{
  if (state_ != State::RUNNING) {
    return 0;
  }

  state_ = State::TERMINATING;
  return 0;
}

int ThreadPoolExecutor::execute(const function<void()> &callable)
{
  unique_ptr<Runnable> task_ptr(new RunnableAdaptor(callable));
  return this->execute(std::move(task_ptr));
}

int ThreadPoolExecutor::execute(unique_ptr<Runnable> &&task)
{
  if (state_ != State::RUNNING) {
    LOG_WARN("[%s] cannot submit task. state=%d", pool_name_.c_str(), state_);
    return -1;
  }

  int ret       = work_queue_->push(std::move(task));
  int task_size = work_queue_->size();
  if (task_size > pool_size() - active_count()) {
    extend_thread();
  }
  return ret;
}

int ThreadPoolExecutor::await_termination()
{
  if (state_ != State::TERMINATING) {
    return -1;
  }

  while (threads_.size() > 0) {
    this_thread::sleep_for(200ms);
  }
  return 0;
}

void ThreadPoolExecutor::thread_func()
{
  LOG_INFO("[%s] thread started", pool_name_.c_str());

  int ret = thread_set_name(pool_name_.c_str());
  if (ret != 0) {
    LOG_WARN("[%s] set thread name failed", pool_name_.c_str());
  }

  lock_.lock();
  auto iter = threads_.find(this_thread::get_id());
  if (iter == threads_.end()) {
    LOG_WARN("[%s] cannot find thread state of %lx", pool_name_.c_str(), this_thread::get_id());
    return;
  }
  ThreadData &thread_data = iter->second;
  lock_.unlock();

  using Clock = chrono::steady_clock;

  chrono::time_point<Clock> idle_deadline = Clock::now();
  if (!thread_data.core_thread && keep_alive_time_ms_.count() > 0) {
    idle_deadline += keep_alive_time_ms_;
  }

  /// 这里使用最粗暴的方式检测线程是否可以退出了
  /// 但是实际上，如果当前的线程个数比任务数要多，或者差不多，而且任务执行都很快的时候，
  /// 并不需要保留这么多线程
  while (thread_data.core_thread || Clock::now() < idle_deadline) {
    unique_ptr<Runnable> task;

    int ret = work_queue_->pop(task);
    if (0 == ret && task) {
      thread_data.idle = false;
      ++active_count_;
      task->run();
      --active_count_;
      thread_data.idle = true;
      ++task_count_;

      if (keep_alive_time_ms_.count() > 0) {
        idle_deadline = Clock::now() + keep_alive_time_ms_;
      }
    }
    if (state_ != State::RUNNING && work_queue_->size() == 0) {
      break;
    }
  }

  thread_data.terminated = true;
  thread_data.thread_ptr->detach();
  delete thread_data.thread_ptr;
  thread_data.thread_ptr = nullptr;

  lock_.lock();
  threads_.erase(this_thread::get_id());
  lock_.unlock();

  LOG_INFO("[%s] thread exit", pool_name_.c_str());
}

int ThreadPoolExecutor::create_thread(bool core_thread)
{
  lock_guard guard(lock_);
  return create_thread_locked(core_thread);
}

int ThreadPoolExecutor::create_thread_locked(bool core_thread)
{
  thread *thread_ptr = new (nothrow) thread(&ThreadPoolExecutor::thread_func, this);
  if (thread_ptr == nullptr) {
    LOG_ERROR("create thread failed");
    return -1;
  }

  ThreadData thread_data;
  thread_data.core_thread        = core_thread;
  thread_data.idle               = true;
  thread_data.terminated         = false;
  thread_data.thread_ptr         = thread_ptr;
  threads_[thread_ptr->get_id()] = thread_data;

  if (static_cast<int>(threads_.size()) > largest_pool_size_) {
    largest_pool_size_ = static_cast<int>(threads_.size());
  }
  return 0;
}

int ThreadPoolExecutor::extend_thread()
{
  lock_guard guard(lock_);

  // 超过最大线程数，不再创建
  if (pool_size() >= max_pool_size_) {
    return 0;
  }
  // 任务数比空闲线程数少，不创建新线程
  if (work_queue_->size() <= pool_size() - active_count()) {
    return 0;
  }

  return create_thread_locked(false /*core_thread*/);
}

}  // end namespace common
