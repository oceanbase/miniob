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

#ifndef __COMMON_LANG_MUTEX_H__
#define __COMMON_LANG_MUTEX_H__

#include <errno.h>
#include <map>
#include <pthread.h>
#include <set>
#include <sstream>
#include <string.h>
#include <string>
#include <sys/types.h>

#include "common/log/log.h"

namespace common {

#define MUTEX_LOG LOG_DEBUG

class LockTrace {
public:
  static void check(pthread_mutex_t *mutex, const long long threadId, const char *file, const int line);
  static void lock(pthread_mutex_t *mutex, const long long threadId, const char *file, const int line);
  static void tryLock(pthread_mutex_t *mutex, const long long threadId, const char *file, const int line);
  static void unlock(pthread_mutex_t *mutex, const long long threadId, const char *file, const int line);

  static void toString(std::string &result);

  class LockID {
  public:
    LockID(const long long threadId, const char *file, const int line) : mFile(file), mThreadId(threadId), mLine(line)
    {}
    LockID() : mFile(), mThreadId(0), mLine(0)
    {}

    std::string toString()
    {
      std::ostringstream oss;

      oss << "threaId:" << mThreadId << ",file name:" << mFile << ",line:" << mLine;

      return oss.str();
    }

  public:
    std::string mFile;
    const long long mThreadId;
    int mLine;
  };

  static void foundDeadLock(LockID &current, LockID &other, pthread_mutex_t *otherWaitMutex);

  static bool deadlockCheck(LockID &current, std::set<pthread_mutex_t *> &ownMutexs, LockID &other, int recusiveNum);

  static bool deadlockCheck(pthread_mutex_t *mutex, const long long threadId, const char *file, const int line);

  static bool checkLockTimes(pthread_mutex_t *mutex, const char *file, const int line);

  static void insertLock(pthread_mutex_t *mutex, const long long threadId, const char *file, const int line);

  static void setMaxBlockThreads(int blockNum)
  {
    mMaxBlockTids = blockNum;
  }

public:
  static std::set<pthread_mutex_t *> mEnableRecurisives;

protected:
  static std::map<pthread_mutex_t *, LockID> mLocks;
  static std::map<pthread_mutex_t *, int> mWaitTimes;
  static std::map<long long, pthread_mutex_t *> mWaitLocks;
  static std::map<long long, std::set<pthread_mutex_t *>> mOwnLocks;

  static pthread_rwlock_t mMapMutex;
  static int mMaxBlockTids;
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

}  // namespace common
#endif  // __COMMON_LANG_MUTEX_H__
