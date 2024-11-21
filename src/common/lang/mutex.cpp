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

#include "common/lang/thread.h"
#include "common/lang/mutex.h"
#include "common/log/log.h"
namespace common {

map<pthread_mutex_t *, LockTrace::LockID>   LockTrace::mLocks;
map<pthread_mutex_t *, int>                 LockTrace::mWaitTimes;
map<long long, pthread_mutex_t *>           LockTrace::mWaitLocks;
map<long long, set<pthread_mutex_t *>> LockTrace::mOwnLocks;
set<pthread_mutex_t *>                      LockTrace::mEnableRecurisives;

pthread_rwlock_t LockTrace::mMapMutex     = PTHREAD_RWLOCK_INITIALIZER;
int              LockTrace::mMaxBlockTids = 8;

#define CHECK_UNLOCK 0

void LockTrace::foundDeadLock(LockID &current, LockTrace::LockID &other, pthread_mutex_t *otherWaitMutex)
{
  map<pthread_mutex_t *, LockTrace::LockID>::iterator itLocks = mLocks.find(otherWaitMutex);
  if (itLocks == mLocks.end()) {
    LOG_ERROR("Thread %ld own mutex %p and try to get mutex %s:%d, "
              "other thread %ld own mutex %s:%d and try to get %p",
        current.mThreadId,
        otherWaitMutex,
        current.mFile.c_str(),
        current.mLine,
        other.mThreadId,
        current.mFile.c_str(),
        current.mLine,
        otherWaitMutex);
  } else {
    LockTrace::LockID &otherRecusive = itLocks->second;

    LOG_ERROR("Thread %ld own mutex %p:%s:%d and try to get mutex %s:%d, "
              "other thread %ld own mutex %s:%d and try to get %p:%s:%d",
        current.mThreadId,
        otherWaitMutex,
        otherRecusive.mFile.c_str(),
        otherRecusive.mLine,
        current.mFile.c_str(),
        current.mLine,
        other.mThreadId,
        current.mFile.c_str(),
        current.mLine,
        otherWaitMutex,
        otherRecusive.mFile.c_str(),
        otherRecusive.mLine);
  }
}

bool LockTrace::deadlockCheck(
    LockID &current, set<pthread_mutex_t *> &ownMutexs, LockTrace::LockID &other, int recusiveNum)
{
  if (recusiveNum >= mMaxBlockTids) {
    return false;
  }

  map<long long, pthread_mutex_t *>::iterator otherIt = mWaitLocks.find(other.mThreadId);
  if (otherIt == mWaitLocks.end()) {
    return false;
  }
  pthread_mutex_t *otherWaitMutex = otherIt->second;

  if (ownMutexs.find(otherWaitMutex) != ownMutexs.end()) {
    // dead lock
    foundDeadLock(current, other, otherWaitMutex);
    return true;
  }

  map<pthread_mutex_t *, LockTrace::LockID>::iterator itLocks = mLocks.find(otherWaitMutex);
  if (itLocks == mLocks.end()) {
    return false;
  }
  LockTrace::LockID &otherRecusive = itLocks->second;

  return deadlockCheck(current, ownMutexs, otherRecusive, recusiveNum + 1);
}

bool LockTrace::deadlockCheck(pthread_mutex_t *mutex, const long long threadId, const char *file, const int line)
{
  mWaitLocks[threadId] = mutex;

  map<pthread_mutex_t *, LockTrace::LockID>::iterator itLocks = mLocks.find(mutex);
  if (itLocks == mLocks.end()) {
    return false;
  }
  LockTrace::LockID &other = itLocks->second;
  if (threadId == other.mThreadId) {
    // lock by himself
    if (mEnableRecurisives.find(mutex) != mEnableRecurisives.end()) {
      // the mutex's attributes has been set recurisves
      return false;
    } else {
      LockID current(threadId, file, line);
      foundDeadLock(current, other, mutex);
      return true;
    }
  }

  map<long long, set<pthread_mutex_t *>>::iterator it = mOwnLocks.find(threadId);
  if (it == mOwnLocks.end()) {
    return false;
  }
  set<pthread_mutex_t *> &ownMutexs = it->second;
  if (ownMutexs.empty() == true) {
    return false;
  }

  LockID current(threadId, file, line);
  return deadlockCheck(current, ownMutexs, other, 1);
}

bool LockTrace::checkLockTimes(pthread_mutex_t *mutex, const char *file, const int line)
{
  map<pthread_mutex_t *, int>::iterator it = mWaitTimes.find(mutex);
  if (it == mWaitTimes.end()) {
    mWaitTimes.insert(pair<pthread_mutex_t *, int>(mutex, 1));

    return false;
  }

  int lockTimes     = it->second;
  mWaitTimes[mutex] = lockTimes + 1;
  if (lockTimes >= mMaxBlockTids) {

    // string          lastLockId = lockId.toString();
    LockTrace::LockID &lockId = mLocks[mutex];
    LOG_WARN("mutex %p has been already lock %d times, this time %s:%d, first "
             "time:%ld:%s:%d",
        mutex,
        lockTimes,
        file,
        line,
        lockId.mThreadId,
        lockId.mFile.c_str(),
        lockId.mLine);

    return true;
  } else {
    return false;
  }
}

void LockTrace::check(pthread_mutex_t *mutex, const long long threadId, const char *file, const int line)
{
  MUTEX_LOG("Lock mutex %p, %s:%d", mutex, file, line);
  pthread_rwlock_rdlock(&mMapMutex);

  deadlockCheck(mutex, threadId, file, line);

  checkLockTimes(mutex, file, line);

  pthread_rwlock_unlock(&mMapMutex);
}

void LockTrace::insertLock(pthread_mutex_t *mutex, const long long threadId, const char *file, const int line)
{
  LockID lockID(threadId, file, line);

  mLocks.insert(pair<pthread_mutex_t *, LockID>(mutex, lockID));

  mWaitLocks.erase(threadId);

  // add entry to mOwnLocks
  set<pthread_mutex_t *> &ownLockSet = mOwnLocks[threadId];
  ownLockSet.insert(mutex);

  map<pthread_mutex_t *, int>::iterator itTimes = mWaitTimes.find(mutex);
  if (itTimes == mWaitTimes.end()) {
    LOG_ERROR("No entry of %p:%s:%d in mWaitTimes", mutex, file, line);

  } else {
    mWaitTimes[mutex] = itTimes->second - 1;
  }
}

void LockTrace::lock(pthread_mutex_t *mutex, const long long threadId, const char *file, const int line)
{
  pthread_rwlock_wrlock(&mMapMutex);

  insertLock(mutex, threadId, file, line);
  pthread_rwlock_unlock(&mMapMutex);
}

void LockTrace::tryLock(pthread_mutex_t *mutex, const long long threadId, const char *file, const int line)
{
  pthread_rwlock_wrlock(&mMapMutex);
  if (mLocks.find(mutex) != mLocks.end()) {
    pthread_rwlock_unlock(&mMapMutex);
    return;
  }

  insertLock(mutex, threadId, file, line);
  pthread_rwlock_unlock(&mMapMutex);
}

void LockTrace::unlock(pthread_mutex_t *mutex, long long threadId, const char *file, int line)
{
  pthread_rwlock_wrlock(&mMapMutex);

  mLocks.erase(mutex);

  set<pthread_mutex_t *> &ownLockSet = mOwnLocks[threadId];
  ownLockSet.erase(mutex);

  pthread_rwlock_unlock(&mMapMutex);
}

void LockTrace::toString(string &result)
{

  const int TEMP_PAIR_LEN = 24;
  // pthread_mutex_lock(&mMapMutex);
  result = " mLocks:\n";
  for (map<pthread_mutex_t *, LockID>::iterator it = mLocks.begin(); it != mLocks.end(); it++) {
    result += it->second.toString();

    char pointerBuf[TEMP_PAIR_LEN] = {0};
    snprintf(pointerBuf, TEMP_PAIR_LEN, ",mutex:%p\n", it->first);

    result += pointerBuf;
  }

  result += "mWaitTimes:\n";
  for (map<pthread_mutex_t *, int>::iterator it = mWaitTimes.begin(); it != mWaitTimes.end(); it++) {
    char pointerBuf[TEMP_PAIR_LEN] = {0};
    snprintf(pointerBuf, TEMP_PAIR_LEN, ",mutex:%p, times:%d\n", it->first, it->second);
    result += pointerBuf;
  }

  result += "mWaitLocks:\n";
  for (map<long long, pthread_mutex_t *>::iterator it = mWaitLocks.begin(); it != mWaitLocks.end(); it++) {
    char pointerBuf[TEMP_PAIR_LEN] = {0};
    snprintf(pointerBuf,
        TEMP_PAIR_LEN,
        "threadID: %llx"
        ", mutex:%p\n",
        it->first,
        it->second);
    result += pointerBuf;
  }
  // pthread_mutex_unlock(&mMapMutex);
  // skip mOwnLocks output

  return;
}

void DebugMutex::lock()
{
#ifdef DEBUG
  lock_.lock();
  LOG_DEBUG("debug lock %p, lbt=%s", &lock_, lbt());
#endif
}

void DebugMutex::unlock()
{
#ifdef DEBUG
  LOG_DEBUG("debug unlock %p, lbt=%s", &lock_, lbt());
  lock_.unlock();
#endif
}

////////////////////////////////////////////////////////////////////////////////
void Mutex::lock()
{
#ifdef CONCURRENCY
  lock_.lock();
  LOG_DEBUG("lock %p, lbt=%s", &lock_, lbt());
#endif
}

bool Mutex::try_lock()
{
#ifdef CONCURRENCY
  bool result = lock_.try_lock();
  if (result) {
    LOG_DEBUG("try lock success %p, lbt=%s", &lock_, lbt());
  }
  return result;
#else
  return true;
#endif
}

void Mutex::unlock()
{
#ifdef CONCURRENCY
  LOG_DEBUG("unlock %p, lbt=%s", &lock_, lbt());
  lock_.unlock();
#endif
}

////////////////////////////////////////////////////////////////////////////////
#ifdef CONCURRENCY

void SharedMutex::lock()
{
  lock_.lock();
  LOG_DEBUG("shared lock %p, lbt=%s", &lock_, lbt());
}
bool SharedMutex::try_lock()
{
  bool result = lock_.try_lock();
  if (result) {
    LOG_DEBUG("try shared lock : %p, lbt=%s", &lock_, lbt());
  }
  return result;
}
void SharedMutex::unlock()  // unlock exclusive
{
  LOG_DEBUG("shared lock unlock %p, lbt=%s", &lock_, lbt());
  lock_.unlock();
}

void SharedMutex::lock_shared()
{
  lock_.lock_shared();
  LOG_DEBUG("shared lock shared: %p lbt=%s", &lock_, lbt());
}
bool SharedMutex::try_lock_shared()
{
  bool result = lock_.try_lock_shared();
  if (result) {
    LOG_DEBUG("shared lock try lock shared: %p, lbt=%s", &lock_, lbt());
  }
  return result;
}
void SharedMutex::unlock_shared()
{
  LOG_DEBUG("shared lock unlock shared %p lbt=%s", &lock_, lbt());
  lock_.unlock_shared();
}

#else  // CONCURRENCY undefined

void SharedMutex::lock() {}
bool SharedMutex::try_lock() { return true; }
void SharedMutex::unlock()  // unlock exclusive
{}

void SharedMutex::lock_shared() {}
bool SharedMutex::try_lock_shared() { return true; }
void SharedMutex::unlock_shared() {}

#endif  // CONCURRENCY end

////////////////////////////////////////////////////////////////////////////////
#ifndef CONCURRENCY
void RecursiveSharedMutex::lock_shared() {}

bool RecursiveSharedMutex::try_lock_shared() { return true; }

void RecursiveSharedMutex::unlock_shared() {}

void RecursiveSharedMutex::lock() {}

void RecursiveSharedMutex::unlock() {}

#else   // ifdef CONCURRENCY

void RecursiveSharedMutex::lock_shared()
{
  unique_lock<mutex> lock(mutex_);
  while (exclusive_lock_count_ > 0) {
    shared_lock_cv_.wait(lock);
  }
  shared_lock_count_++;
}

bool RecursiveSharedMutex::try_lock_shared()
{
  unique_lock<mutex> lock(mutex_);
  if (exclusive_lock_count_ == 0) {
    shared_lock_count_++;
    return true;
  }
  return false;
}

void RecursiveSharedMutex::unlock_shared()
{
  unique_lock<mutex> lock(mutex_);
  shared_lock_count_--;
  if (shared_lock_count_ == 0) {
    exclusive_lock_cv_.notify_one();
  }
}

void RecursiveSharedMutex::lock()
{
  unique_lock<mutex> lock(mutex_);
  while (shared_lock_count_ > 0 || exclusive_lock_count_ > 0) {
    if (recursive_owner_ == this_thread::get_id()) {
      recursive_count_++;
      return;
    }
    exclusive_lock_cv_.wait(lock);
  }
  recursive_owner_ = this_thread::get_id();
  recursive_count_ = 1;
  exclusive_lock_count_++;
}

void RecursiveSharedMutex::unlock()
{
  unique_lock<mutex> lock(mutex_);
  if (recursive_owner_ == this_thread::get_id() && recursive_count_ > 1) {
    recursive_count_--;
  } else {
    recursive_owner_ = thread::id();
    recursive_count_ = 0;
    exclusive_lock_count_--;
    if (exclusive_lock_count_ == 0) {
      shared_lock_cv_.notify_all();
      exclusive_lock_cv_.notify_one();
    }
  }
}
#endif  // CONCURRENCY

}  // namespace common
