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

#include "common/lang/mutex.h"
#include "common/log/log.h"
namespace common {

std::map<pthread_mutex_t *, LockTrace::LockID> LockTrace::mLocks;
std::map<pthread_mutex_t *, int> LockTrace::mWaitTimes;
std::map<long long, pthread_mutex_t *> LockTrace::mWaitLocks;
std::map<long long, std::set<pthread_mutex_t *>> LockTrace::mOwnLocks;
std::set<pthread_mutex_t *> LockTrace::mEnableRecurisives;

pthread_rwlock_t LockTrace::mMapMutex = PTHREAD_RWLOCK_INITIALIZER;
int LockTrace::mMaxBlockTids = 8;

#define CHECK_UNLOCK 0

void LockTrace::foundDeadLock(LockID &current, LockTrace::LockID &other, pthread_mutex_t *otherWaitMutex)
{
  std::map<pthread_mutex_t *, LockTrace::LockID>::iterator itLocks = mLocks.find(otherWaitMutex);
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
    LockID &current, std::set<pthread_mutex_t *> &ownMutexs, LockTrace::LockID &other, int recusiveNum)
{
  if (recusiveNum >= mMaxBlockTids) {
    return false;
  }

  std::map<long long, pthread_mutex_t *>::iterator otherIt = mWaitLocks.find(other.mThreadId);
  if (otherIt == mWaitLocks.end()) {
    return false;
  }
  pthread_mutex_t *otherWaitMutex = otherIt->second;

  if (ownMutexs.find(otherWaitMutex) != ownMutexs.end()) {
    // dead lock
    foundDeadLock(current, other, otherWaitMutex);
    return true;
  }

  std::map<pthread_mutex_t *, LockTrace::LockID>::iterator itLocks = mLocks.find(otherWaitMutex);
  if (itLocks == mLocks.end()) {
    return false;
  }
  LockTrace::LockID &otherRecusive = itLocks->second;

  return deadlockCheck(current, ownMutexs, otherRecusive, recusiveNum + 1);
}

bool LockTrace::deadlockCheck(pthread_mutex_t *mutex, const long long threadId, const char *file, const int line)
{
  mWaitLocks[threadId] = mutex;

  std::map<pthread_mutex_t *, LockTrace::LockID>::iterator itLocks = mLocks.find(mutex);
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

  std::map<long long, std::set<pthread_mutex_t *>>::iterator it = mOwnLocks.find(threadId);
  if (it == mOwnLocks.end()) {
    return false;
  }
  std::set<pthread_mutex_t *> &ownMutexs = it->second;
  if (ownMutexs.empty() == true) {
    return false;
  }

  LockID current(threadId, file, line);
  return deadlockCheck(current, ownMutexs, other, 1);
}

bool LockTrace::checkLockTimes(pthread_mutex_t *mutex, const char *file, const int line)
{
  std::map<pthread_mutex_t *, int>::iterator it = mWaitTimes.find(mutex);
  if (it == mWaitTimes.end()) {
    mWaitTimes.insert(std::pair<pthread_mutex_t *, int>(mutex, 1));

    return false;
  }

  int lockTimes = it->second;
  mWaitTimes[mutex] = lockTimes + 1;
  if (lockTimes >= mMaxBlockTids) {

    // std::string          lastLockId = lockId.toString();
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

  mLocks.insert(std::pair<pthread_mutex_t *, LockID>(mutex, lockID));

  mWaitLocks.erase(threadId);

  // add entry to mOwnLocks
  std::set<pthread_mutex_t *> &ownLockSet = mOwnLocks[threadId];
  ownLockSet.insert(mutex);

  std::map<pthread_mutex_t *, int>::iterator itTimes = mWaitTimes.find(mutex);
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

  std::set<pthread_mutex_t *> &ownLockSet = mOwnLocks[threadId];
  ownLockSet.erase(mutex);

  pthread_rwlock_unlock(&mMapMutex);
}

void LockTrace::toString(std::string &result)
{

  const int TEMP_PAIR_LEN = 24;
  // pthread_mutex_lock(&mMapMutex);
  result = " mLocks:\n";
  for (std::map<pthread_mutex_t *, LockID>::iterator it = mLocks.begin(); it != mLocks.end(); it++) {
    result += it->second.toString();

    char pointerBuf[TEMP_PAIR_LEN] = {0};
    snprintf(pointerBuf, TEMP_PAIR_LEN, ",mutex:%p\n", it->first);

    result += pointerBuf;
  }

  result += "mWaitTimes:\n";
  for (std::map<pthread_mutex_t *, int>::iterator it = mWaitTimes.begin(); it != mWaitTimes.end(); it++) {
    char pointerBuf[TEMP_PAIR_LEN] = {0};
    snprintf(pointerBuf, TEMP_PAIR_LEN, ",mutex:%p, times:%d\n", it->first, it->second);
    result += pointerBuf;
  }

  result += "mWaitLocks:\n";
  for (std::map<long long, pthread_mutex_t *>::iterator it = mWaitLocks.begin(); it != mWaitLocks.end(); it++) {
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

}  // namespace common