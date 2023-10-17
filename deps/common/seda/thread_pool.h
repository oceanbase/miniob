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
// Created by Longda on 2010
//

#ifndef __COMMON_SEDA_THREAD_POOL_H__
#define __COMMON_SEDA_THREAD_POOL_H__

#include <deque>

#include "common/defs.h"
#include "common/seda/kill_thread.h"
namespace common {

class Stage;

/**
 * A thread pool for one or more seda stages
 * The Threadpool class consists of a pool of worker threads and a
 * scheduling queue of active seda Stages that have events that need
 * processing.  Each thread in the thread pool constantly examines the
 * head of the scheduling queue for a scheduled stage.  It then removes
 * the scheduled stage from the queue, and selects an event from the
 * Stage's event queue for processing. The thread then processes the event
 * using the Stage's handle_event() member function before starting the
 * process over.  If the thread finds the scheduling queue empty, it sleeps
 * on a condition waiting for Stages to schedule themselves.
 * <p>
 * The number of threads in the pool can be controlled by clients. On
 * creation, the caller provides a parameter indicating the initial number
 * of worker threads, but this number can be adjusted at any time by using
 * the add_threads(), num_threads(), and kill_threads() interfaces.
 */
class Threadpool {

public:
  // Initialize the static data structures of ThreadPool
  static void create_pool_key();

  // Finalize the static data structures of ThreadPool
  static void del_pool_key();

  /**
   * Constructor
   * @param[in] threads The number of threads to create.
   * @param[in] name    Name of the thread pool.
   *
   * @post thread pool has <i>threads</i> threads running
   */
  Threadpool(unsigned int threads, const std::string &name = std::string());

  /**
   * Destructor
   * Kills all threads and destroys pool.
   *
   * @post all threads are destroyed and pool is destroyed
   */
  virtual ~Threadpool();

  /**
   * Query number of threads.
   * @return number of threads in the thread pool.
   */
  unsigned int num_threads();

  /**
   * Add threads to the pool
   * @param[in] threads Number of threads to add to the pool.
   *
   * @post  0 <= (# of threads in pool) - (original # of threads in pool)
   *        <= threads
   * @return number of thread successfully created
   */
  unsigned int add_threads(unsigned int threads);

  /**
   * Kill threads in pool
   * Blocks until the requested number of threads are killed.  Won't
   * kill more than current number of threads.
   *
   * @param[in] threads Number of threads to kill.
   *
   * @post (original # of threads in pool) - (# of threads in pool)
   *       <= threads
   * @return number of threads successfully killed.
   */
  unsigned int kill_threads(unsigned int threads);

  /**
   * Schedule stage with some work
   * Schedule a stage with some work to be done on the run queue.
   *
   * @param[in] stage Reference to stage to be scheduled.
   *
   * @pre  stage must have a non-empty queue.
   * @post stage is scheduled on the run queue.
   */
  void schedule(Stage *stage);

  // Get name of thread pool
  const std::string &get_name();

protected:
  /**
   * Internal thread kill.
   * Internal operation called only when a thread kill event is processed.
   * Reduces the count of active threads, and, if this is the last pending
   * kill, signals the waiting kill_threads method.
   */
  void thread_kill();

  /**
   * Internal generate kill thread events
   * Internal operation called by kill_threads(). Generates the requested
   * number of kill thread events and schedules them.
   *
   * @pre  thread mutex is locked.
   * @pre  to_kill <= current number of threads
   * @return number of kill thread events successfully scheduled
   */
  unsigned int gen_kill_thread_events(unsigned int to_kill);

private:
  /**
   * Internal thread control function
   * Function which contains the control loop for each service thread.
   * Should not be called except when a thread is created.
   */
  static void *run_thread(void *pool_ptr);

  // Save the thread pool pointer for this thread
  static void set_thread_pool_ptr(const Threadpool *thd_pool);

  // Get the thread pool pointer for this thread
  static const Threadpool *get_thread_pool_ptr();

  // run queue state
  pthread_mutex_t run_mutex_;      //< protects the run queue
  pthread_cond_t run_cond_;        //< wait here for stage to be scheduled
  std::deque<Stage *> run_queue_;  //< list of stages with work to do
  bool eventhist_;                 //< is event history enabled?

  // thread state
  pthread_mutex_t thread_mutex_;  //< protects thread state
  pthread_cond_t thread_cond_;    //< wait here when killing threads
  unsigned int nthreads_;         //< number of service threads
  unsigned int threads_to_kill_;  //< number of pending kill events
  unsigned int n_idles_;          //< number of idle threads
  KillThreadStage killer_;        //< used to kill threads
  std::string name_;              //< name of threadpool

  // key of thread specific to store thread pool pointer
  static pthread_key_t pool_ptr_key_;

  // allow KillThreadStage to kill threads
  friend class KillThreadStage;
};

}  // namespace common
#endif  // __COMMON_SEDA_THREAD_POOL_H__
