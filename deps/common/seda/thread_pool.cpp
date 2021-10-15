/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
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

// Include Files
#include "common/seda/thread_pool.h"

#include <assert.h>

#include "common/lang/mutex.h"
#include "common/log/log.h"
#include "common/seda/stage.h"
namespace common {

extern bool &get_event_history_flag();

/**
 * Constructor
 * @param[in] threads The number of threads to create.
 *
 * @post thread pool has <i>threads</i> threads running
 */
Threadpool::Threadpool(unsigned int threads, const std::string &name)
  : run_queue_(), eventhist_(get_event_history_flag()), nthreads_(0),
    threads_to_kill_(0), n_idles_(0), killer_("KillThreads"), name_(name) {
  LOG_TRACE("Enter, thread number:%d", threads);
  MUTEX_INIT(&run_mutex_, NULL);
  COND_INIT(&run_cond_, NULL);
  MUTEX_INIT(&thread_mutex_, NULL);
  COND_INIT(&thread_cond_, NULL);
  add_threads(threads);
  LOG_TRACE("exit");
}

/**
 * Destructor
 * Kills all threads and destroys pool.
 *
 * @post all threads are destroyed and pool is destroyed
 */
Threadpool::~Threadpool() {
  LOG_TRACE("%s", "enter");
  // kill all the remaining service threads
  kill_threads(nthreads_);

  run_queue_.clear();
  MUTEX_DESTROY(&run_mutex_);
  COND_DESTROY(&run_cond_);
  MUTEX_DESTROY(&thread_mutex_);
  COND_DESTROY(&thread_cond_);
  LOG_TRACE("%s", "exit");
}

/**
 * Query number of threads.
 * @return number of threads in the thread pool.
 */
unsigned int Threadpool::num_threads() {
  MUTEX_LOCK(&thread_mutex_);
  unsigned int result = nthreads_;
  MUTEX_UNLOCK(&thread_mutex_);
  return result;
}

/**
 * Add threads to the pool
 * @param[in] threads Number of threads to add to the pool.
 *
 * @post  0 <= (# of threads in pool) - (original # of threads in pool)
 *        <= threads
 * @return number of thread successfully created
 */
unsigned int Threadpool::add_threads(unsigned int threads) {
  unsigned int i;
  pthread_t pthread;
  pthread_attr_t pthread_attrs;
  LOG_TRACE("%s adding threads enter%d", name_.c_str(), threads);
  // create all threads as detached.  We will not try to join them.
  pthread_attr_init(&pthread_attrs);
  pthread_attr_setdetachstate(&pthread_attrs, PTHREAD_CREATE_DETACHED);

  MUTEX_LOCK(&thread_mutex_);

  // attempt to start the requested number of threads
  for (i = 0; i < threads; i++) {
    int stat = pthread_create(&pthread, &pthread_attrs, Threadpool::run_thread,
                              (void *) this);
    if (stat != 0) {
      LOG_WARN("Failed to create one thread\n");
      break;
    }
  }
  nthreads_ += i;
  MUTEX_UNLOCK(&thread_mutex_);
  LOG_TRACE("%s%d", "adding threads exit", threads);
  return i;
}

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
unsigned int Threadpool::kill_threads(unsigned int threads) {
  LOG_TRACE("%s%d", "enter - threads to kill", threads);
  MUTEX_LOCK(&thread_mutex_);

  // allow only one thread kill transaction at a time
  if (threads_to_kill_ > 0) {
    MUTEX_UNLOCK(&thread_mutex_);
    return 0;
  }

  // check the limit
  if (threads > nthreads_) {
    threads = nthreads_;
  }

  // connect the kill thread stage to this pool
  killer_.set_pool(this);
  killer_.connect();

  // generate an appropriate number of kill thread events...
  int i = gen_kill_thread_events(threads);

  // set the counter and wait for events to be picked up.
  threads_to_kill_ = i;
  COND_WAIT(&thread_cond_, &thread_mutex_);

  killer_.disconnect();

  MUTEX_UNLOCK(&thread_mutex_);
  LOG_TRACE("%s", "exit");
  return i;
}

/**
 * Internal thread kill.
 * Internal operation called only when a thread kill event is processed.
 * Reduces the count of active threads, and, if this is the last pending
 * kill, signals the waiting kill_threads method.
 */
void Threadpool::thread_kill() {
  MUTEX_LOCK(&thread_mutex_);

  nthreads_--;
  threads_to_kill_--;
  if (threads_to_kill_ == 0) {
    // signal the condition, in case someone is waiting there...
    COND_SIGNAL(&thread_cond_);
  }

  MUTEX_UNLOCK(&thread_mutex_);
}

/**
 * Internal generate kill thread events
 * Internal operation called by kill_threads(). Generates the requested
 * number of kill thread events and schedules them.
 *
 * @pre  thread mutex is locked.
 * @pre  to_kill <= current number of threads
 * @return number of kill thread events successfully scheduled
 */
unsigned int Threadpool::gen_kill_thread_events(unsigned int to_kill) {
  LOG_TRACE("%s%d", "enter", to_kill);
  assert(MUTEX_TRYLOCK(&thread_mutex_) != 0);
  assert(to_kill <= nthreads_);

  unsigned int i;
  for (i = 0; i < to_kill; i++) {

    // allocate kill thread event and put it on the list...
    StageEvent *sevent = new StageEvent();
    if (sevent == NULL) {
      break;
    }
    killer_.add_event(sevent);
  }
  LOG_TRACE("%s%d", "exit", to_kill);
  return i;
}

/**
 * Schedule stage with some work
 * Schedule a stage with some work to be done on the run queue.
 *
 * @param[in] stage Reference to stage to be scheduled.
 *
 * @pre  stage must have a non-empty queue.
 * @post stage is scheduled on the run queue.
 */
void Threadpool::schedule(Stage *stage) {
  assert(!stage->qempty());

  MUTEX_LOCK(&run_mutex_);
  bool was_empty = run_queue_.empty();
  run_queue_.push_back(stage);
  // let current thread continue to run the target stage if there is
  // only one event and the target stage is in the same thread pool
  if (was_empty == false || this != get_thread_pool_ptr()) {
    // wake up if there is idle thread
    if (n_idles_ > 0) {
      COND_SIGNAL(&run_cond_);
    }
  }
  MUTEX_UNLOCK(&run_mutex_);
}

// Get name of thread pool
const std::string &Threadpool::get_name() { return name_; }

/**
 * Internal thread control function
 * Function which contains the control loop for each service thread.
 * Should not be called except when a thread is created.
 */
void *Threadpool::run_thread(void *pool_ptr) {
  Threadpool *pool = (Threadpool *) pool_ptr;

  // save thread pool pointer
  set_thread_pool_ptr(pool);

  // this is not portable, but is easier to map to LWP
  s64_t threadid = gettid();
  LOG_INFO("threadid = %llx, threadname = %s\n", threadid,
           pool->get_name().c_str());

  // enter a loop where we continuously look for events from Stages on
  // the run_queue_ and handle the event.
  while (1) {
    MUTEX_LOCK(&(pool->run_mutex_));

    // wait for some stage to be scheduled
    while (pool->run_queue_.empty()) {
      (pool->n_idles_)++;
      COND_WAIT(&(pool->run_cond_), &(pool->run_mutex_));
      (pool->n_idles_)--;
    }

    assert(!pool->run_queue_.empty());
    Stage *run_stage = *(pool->run_queue_.begin());
    pool->run_queue_.pop_front();
    MUTEX_UNLOCK(&(pool->run_mutex_));

    StageEvent *event = run_stage->remove_event();

    // need to check if this is a rescheduled callback
    if (event->is_callback()) {
#ifdef ENABLE_STAGE_LEVEL_TIMEOUT
      // check if the event has timed out.
      if (event->has_timed_out()) {
        event->done_timeout();
      } else {
        event->done_immediate();
      }
#else
      event->done_immediate();
#endif
    } else {
      if (pool->eventhist_) {
        event->save_stage(run_stage, StageEvent::HANDLE_EV);
      }

#ifdef ENABLE_STAGE_LEVEL_TIMEOUT
      // check if the event has timed out
      if (event->has_timed_out()) {
        event->done();
      } else {
        run_stage->handle_event(event);
      }
#else
      run_stage->handle_event(event);
#endif
    }
    run_stage->release_event();
  }
  LOG_TRACE("exit %p", pool_ptr);
  LOG_INFO("Begin to exit, threadid = %llx, threadname = %s", threadid,
           pool->get_name().c_str());

  // the dummy compiler need this
  pthread_exit(NULL);
}

pthread_key_t Threadpool::pool_ptr_key_;

void Threadpool::create_pool_key() {
  // init the thread specific to store thread pool pointer
  // this is called in main thread, so no pthread_once is needed
  pthread_key_create(&pool_ptr_key_, NULL);
}

void Threadpool::del_pool_key() { pthread_key_delete(pool_ptr_key_); }

void Threadpool::set_thread_pool_ptr(const Threadpool *thd_Pool) {
  pthread_setspecific(pool_ptr_key_, thd_Pool);
}

const Threadpool *Threadpool::get_thread_pool_ptr() {
  return (const Threadpool *) pthread_getspecific(pool_ptr_key_);
}

} //namespace common