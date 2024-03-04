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
   * @param core_size 核心线程个数。核心线程不会退出
   * @param max_size  线程池最大线程个数
   * @param keep_alive_time_ms 非核心线程空闲多久后退出
   */
  int init(const char *name, int core_size, int max_size, long keep_alive_time_ms);

  /**
   * @brief 初始化线程池
   *
   * @param name 线程池名称
   * @param core_size 核心线程个数。核心线程不会退出
   * @param max_size  线程池最大线程个数
   * @param keep_alive_time_ms 非核心线程空闲多久后退出
   * @param work_queue 任务队列
   */
  int init(const char *name, int core_pool_size, int max_pool_size, long keep_alive_time_ms,
      std::unique_ptr<Queue<std::unique_ptr<Runnable>>> &&work_queue);

  /**
   * @brief 提交一个任务，不一定可以立即执行
   *
   * @param task 任务
   * @return int 成功放入队列返回0
   */
  int execute(std::unique_ptr<Runnable> &&task);

  /**
   * @brief 提交一个任务，不一定可以立即执行
   *
   * @param callable 任务
   * @return int 成功放入队列返回0
   */
  int execute(const std::function<void()> &callable);

  /**
   * @brief 关闭线程池
   */
  int shutdown();
  /**
   * @brief 等待线程池处理完所有任务并退出
   */
  int await_termination();

public:
  /**
   * @brief 当前活跃线程的个数，就是正在处理任务的线程个数
   */
  int active_count() const { return active_count_.load(); }
  /**
   * @brief 核心线程个数
   */
  int core_pool_size() const { return core_pool_size_; }
  /**
   * @brief 线程池中线程个数
   */
  int pool_size() const { return static_cast<int>(threads_.size()); }
  /**
   * @brief 曾经达到过的最大线程个数
   */
  int largest_pool_size() const { return largest_pool_size_; }
  /**
   * @brief 处理过的任务个数
   */
  int64_t task_count() const { return task_count_.load(); }

private:
  /**
   * @brief 创建一个线程
   *
   * @param core_thread 是否是核心线程
   */
  int create_thread(bool core_thread);
  /**
   * @brief 创建一个线程。调用此函数前已经加锁
   *
   * @param core_thread 是否是核心线程
   */
  int create_thread_locked(bool core_thread);
  /**
   * @brief 检测是否需要扩展线程，如果需要就扩展
   */
  int extend_thread();

private:
  /**
   * @brief 线程函数。从队列中拉任务并执行
   */
  void thread_func();

private:
  /**
   * @brief 线程池的状态
   */
  enum class State
  {
    NEW,          //! 新建状态
    RUNNING,      //! 正在运行
    TERMINATING,  //! 正在停止
    TERMINATED    //! 已经停止
  };

  struct ThreadData
  {
    bool         core_thread = false;    /// 是否是核心线程
    bool         idle        = false;    /// 是否空闲
    bool         terminated  = false;    /// 是否已经退出
    std::thread *thread_ptr  = nullptr;  /// 线程指针
  };

private:
  State state_ = State::NEW;  /// 线程池状态

  int                       core_pool_size_ = 0;  /// 核心线程个数
  int                       max_pool_size_  = 0;  /// 最大线程个数
  std::chrono::milliseconds keep_alive_time_ms_;  /// 非核心线程空闲多久后退出

  std::unique_ptr<Queue<std::unique_ptr<Runnable>>> work_queue_;  /// 任务队列

  mutable std::mutex                    lock_;     /// 保护线程池内部数据的锁
  std::map<std::thread::id, ThreadData> threads_;  /// 线程列表

  int                  largest_pool_size_ = 0;  /// 历史上达到的最大的线程个数
  std::atomic<int64_t> task_count_        = 0;  /// 处理过的任务个数
  std::atomic<int>     active_count_      = 0;  /// 活跃线程个数
  std::string          pool_name_;              /// 线程池名称
};

}  // namespace common
