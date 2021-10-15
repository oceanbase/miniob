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

#ifndef __COMMON_MM_MPOOL_H__
#define __COMMON_MM_MPOOL_H__

#include <queue>

#include "common/lang/mutex.h"
#include "common/log/log.h"
namespace common {

#define CLMPOOL_DEFAULT_ADDSIZE 16

template<class T>
class MemPool {
 public:
  MemPool() : mQueue(), mAddSize(CLMPOOL_DEFAULT_ADDSIZE) {
    MUTEX_INIT(&mLock, NULL);
  }

  ~MemPool() {
    MUTEX_LOCK(&mLock);
    while (mQueue.empty() == false) {
      T *item = mQueue.front();
      mQueue.pop();
      delete item;
    }

    MUTEX_UNLOCK(&mLock);
    MUTEX_DESTROY(&mLock);
  }

  int add(int addSize) {
    int ret = 0;

    MUTEX_LOCK(&mLock);
    for (int i = 0; i < addSize; i++) {
      T *item = new T();
      if (item == NULL) {
        ret = -1;
        break;
      }
      mQueue.push(item);
    }
    MUTEX_UNLOCK(&mLock);

    return ret;
  }

  int init(int initSize) { return add(initSize); }

  T *get() {
    T *ret = NULL;

    MUTEX_LOCK(&mLock);
    if (mQueue.empty() == true) {
      add(mAddSize);
    }

    if (mQueue.empty() == false) {
      ret = mQueue.front();
      mQueue.pop();
    }

    MUTEX_UNLOCK(&mLock);

    return ret;
  }

  void put(T *item) {
    MUTEX_LOCK(&mLock);

    mQueue.push(item);

    MUTEX_UNLOCK(&mLock);
  }

 private:
  std::queue<T *> mQueue;
  pthread_mutex_t mLock;
  int mAddSize;
};

} //namespace common
#endif /* __COMMON_MM_MPOOL_H__ */
