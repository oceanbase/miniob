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

#pragma once

#include <condition_variable>
#include <errno.h>
#include <map>
#include <mutex>
#include <pthread.h>
#include <shared_mutex>
#include <sstream>
#include <string.h>
#include <string>
#include <sys/types.h>
#include <unordered_map>

#include "common/lang/thread.h"
#include "common/log/log.h"

using std::call_once;
using std::condition_variable;
using std::lock_guard;
using std::mutex;
using std::once_flag;
using std::scoped_lock;
using std::shared_mutex;
using std::unique_lock;

namespace common {

#define MUTEX_LOG LOG_DEBUG

class LockTrace
{
public:
  static void check(pthread_mutex_t *mutex, const long long threadId, const char *file, const int line);
  static void lock(pthread_mutex_t *mutex, const long long threadId, const char *file, const int line);
  static void tryLock(pthread_mutex_t *mutex, const long long threadId, const char *file, const int line);
  static void unlock(pthread_mutex_t *mutex, const long long threadId, const char *file, const int line);

  static void toString(string &result);

  class LockID
  {
  public:
    LockID(const long long threadId, const char *file, const int line) : mFile(file), mThreadId(threadId), mLine(line)
    {}
    LockID() : mFile(), mThreadId(0), mLine(0) {}

    string toString()
    {
      ostringstream oss;

      oss << "threaId:" << mThreadId << ",file name:" << mFile << ",line:" << mLine;

      return oss.str();
    }

  public:
    string          mFile;
    const long long mThreadId;
    int             mLine;
  };

  static void foundDeadLock(LockID &current, LockID &other, pthread_mutex_t *otherWaitMutex);

  static bool deadlockCheck(LockID &current, set<pthread_mutex_t *> &ownMutexs, LockID &other, int recusiveNum);

  static bool deadlockCheck(pthread_mutex_t *mutex, const long long threadId, const char *file, const int line);

  static bool checkLockTimes(pthread_mutex_t *mutex, const char *file, const int line);

  static void insertLock(pthread_mutex_t *mutex, const long long threadId, const char *file, const int line);

  static void setMaxBlockThreads(int blockNum) { mMaxBlockTids = blockNum; }

public:
  static set<pthread_mutex_t *> mEnableRecurisives;

protected:
  static map<pthread_mutex_t *, LockID>         mLocks;
  static map<pthread_mutex_t *, int>            mWaitTimes;
  static map<long long, pthread_mutex_t *>      mWaitLocks;
  static map<long long, set<pthread_mutex_t *>> mOwnLocks;

  static pthread_rwlock_t mMapMutex;
  static int              mMaxBlockTids;
};

// Open this macro in Makefile
#ifndef DEBUG_LOCK

#define MUTEXT_STATIC_INIT() PTHREAD_MUTEX_INITIALIZER
#define MUTEX_INIT(lock, attr) pthread_mutex_init(lock, attr)
#define MUTEX_DESTROY(lock) pthread_mutex_destroy(lock)
#define MUTEX_LOCK(lock) pthread_mutex_lock(lock)
#define MUTEX_UNLOCK(lock) pthread_mutex_unlock(lock)
#define MUTEX_TRYLOCK(lock) pthread_mutex_trylock(lock)

#define COND_INIT(cond, attr) pthread_cond_init(cond, attr)
#define COND_DESTROY(cond) pthread_cond_destroy(cond)
#define COND_WAIT(cond, mutex) pthread_cond_wait(cond, mutex)
#define COND_WAIT_TIMEOUT(cond, mutex, time, ret) ret = pthread_cond_timedwait(cond, mutex, time)
#define COND_SIGNAL(cond) pthread_cond_signal(cond)
#define COND_BRAODCAST(cond) pthread_cond_broadcast(cond)

#else  // DEBUG_LOCK

#define MUTEX_STATIC_INIT()  \
  PTHREAD_MUTEX_INITIALIZER; \
  LOG_INFO("PTHREAD_MUTEX_INITIALIZER");

#if defined(__MACH__)

#define MUTEX_INIT(lock, attr)                      \
  ({                                                \
    LOG_INFO("pthread_mutex_init %p", lock);        \
    if (attr != NULL) {                             \
      int type;                                     \
      pthread_mutexattr_gettype(attr, &type);       \
      if (type == PTHREAD_MUTEX_RECURSIVE) {        \
        LockTrace::mEnableRecurisives.insert(lock); \
      }                                             \
    }                                               \
    int result = pthread_mutex_init(lock, attr);    \
    result;                                         \
  })

#else

#define MUTEX_INIT(lock, attr)                      \
  ({                                                \
    LOG_INFO("pthread_mutex_init %p", lock);        \
    if (attr != NULL) {                             \
      int type;                                     \
      pthread_mutexattr_gettype(attr, &type);       \
      if (type == PTHREAD_MUTEX_RECURSIVE_NP) {     \
        LockTrace::mEnableRecurisives.insert(lock); \
      }                                             \
    }                                               \
    int result = pthread_mutex_init(lock, attr);    \
    result;                                         \
  })
#endif

#define MUTEX_DESTROY(lock)                     \
  ({                                            \
    LockTrace::mEnableRecurisives.erase(lock);  \
    int result = pthread_mutex_destroy(lock);   \
    LOG_INFO("pthread_mutex_destroy %p", lock); \
    result;                                     \
  })

#define MUTEX_LOCK(mutex)                                                      \
  ({                                                                           \
    LockTrace::check(mutex, gettid(), __FILE__, __LINE__);                     \
    int result = pthread_mutex_lock(mutex);                                    \
    LockTrace::lock(mutex, gettid(), __FILE__, __LINE__);                      \
    if (result) {                                                              \
      LOG_ERROR("Failed to lock %p, rc %d:%s", mutex, errno, strerror(errno)); \
    }                                                                          \
    result;                                                                    \
  })

#define MUTEX_TRYLOCK(mutex)                                \
  ({                                                        \
    LockTrace::check(mutex, gettid(), __FILE__, __LINE__);  \
    int result = pthread_mutex_trylock(mutex);              \
    if (result == 0) {                                      \
      LockTrace::lock(mutex, gettid(), __FILE__, __LINE__); \
    }                                                       \
    result;                                                 \
  })

#define MUTEX_UNLOCK(lock)                                                      \
  ({                                                                            \
    int result = pthread_mutex_unlock(lock);                                    \
    LockTrace::unlock(lock, gettid(), __FILE__, __LINE__);                      \
    MUTEX_LOG("mutex:%p has been ulocked", lock);                               \
    if (result) {                                                               \
      LOG_ERROR("Failed to unlock %p, rc %d:%s", lock, errno, strerror(errno)); \
    }                                                                           \
    result;                                                                     \
  })

#define COND_INIT(cond, attr)                   \
  ({                                            \
    LOG_INFO("pthread_cond_init");              \
    int result = pthread_cond_init(cond, attr); \
    result;                                     \
  })

#define COND_DESTROY(cond)                   \
  ({                                         \
    int result = pthread_cond_destroy(cond); \
    LOG_INFO("pthread_cond_destroy");        \
    result;                                  \
  })

#define COND_WAIT(cond, mutex)                                      \
  ({                                                                \
    MUTEX_LOG("pthread_cond_wait, cond:%p, mutex:%p", cond, mutex); \
    LockTrace::unlock(mutex, gettid(), __FILE__, __LINE__);         \
    int result = pthread_cond_wait(cond, mutex);                    \
    LockTrace::check(mutex, gettid(), __FILE__, __LINE__);          \
    LockTrace::lock(mutex, gettid(), __FILE__, __LINE__);           \
    MUTEX_LOG("Lock %p under pthread_cond_wait", mutex);            \
    result;                                                         \
  })

#define COND_WAIT_TIMEOUT(cond, mutex, time, ret)                        \
  ({                                                                     \
    MUTEX_LOG("pthread_cond_timedwait, cond:%p, mutex:%p", cond, mutex); \
    LockTrace::unlock(mutex, gettid(), __FILE__, __LINE__);              \
    int result = pthread_cond_timedwait(cond, mutex, time);              \
    if (result == 0) {                                                   \
      LockTrace::check(mutex, gettid(), __FILE__, __LINE__);             \
      LockTrace::lock(mutex, gettid(), __FILE__, __LINE__);              \
      MUTEX_LOG("Lock %p under pthread_cond_wait", mutex);               \
    }                                                                    \
    result;                                                              \
  })

#define COND_SIGNAL(cond)                            \
  ({                                                 \
    int result = pthread_cond_signal(cond);          \
    MUTEX_LOG("pthread_cond_signal, cond:%p", cond); \
    result;                                          \
  })

#define COND_BRAODCAST(cond)                            \
  ({                                                    \
    int result = pthread_cond_broadcast(cond);          \
    MUTEX_LOG("pthread_cond_broadcast, cond:%p", cond); \
    result;                                             \
  })

#endif  // DEBUG_LOCK

class DebugMutex final
{
public:
  DebugMutex()  = default;
  ~DebugMutex() = default;

  void lock();
  void unlock();

private:
#ifdef DEBUG
  mutex lock_;
#endif
};

class Mutex final
{
public:
  Mutex()  = default;
  ~Mutex() = default;

  void lock();
  bool try_lock();
  void unlock();

private:
#ifdef CONCURRENCY
  mutex lock_;
#endif
};

class SharedMutex final
{
public:
  SharedMutex()  = default;
  ~SharedMutex() = default;

  void lock();  // lock exclusive
  bool try_lock();
  void unlock();  // unlock exclusive

  void lock_shared();
  bool try_lock_shared();
  void unlock_shared();

private:
#ifdef CONCURRENCY
  shared_mutex lock_;
#endif
};

/**
 * 支持写锁递归加锁的读写锁
 * 读锁本身就可以递归加锁。但是某个线程加了读锁后，也不能再加写锁。
 * 但是一个线程可以加多次写锁
 * 与其它类型的锁一样，在CONCURRENCY编译模式下才会真正的生效
 */
class RecursiveSharedMutex
{
public:
  RecursiveSharedMutex()  = default;
  ~RecursiveSharedMutex() = default;

  void lock_shared();
  bool try_lock_shared();
  void unlock_shared();

  void lock();
  void unlock();

private:
#ifdef CONCURRENCY
  mutex              mutex_;
  condition_variable shared_lock_cv_;
  condition_variable exclusive_lock_cv_;
  int                shared_lock_count_    = 0;
  int                exclusive_lock_count_ = 0;
  thread::id         recursive_owner_;
  int                recursive_count_ = 0;  // 表示当前线程加写锁加了多少次
#endif                                      // CONCURRENCY
};

}  // namespace common
