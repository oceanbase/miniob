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

#include <queue>
#include <list>
#include <set>
#include <string>
#include <sstream>
#include <functional>
#include <memory>

#include "common/lang/mutex.h"
#include "common/log/log.h"
#include "common/os/os.h"

namespace common {

#define DEFAULT_ITEM_NUM_PER_POOL 128
#define DEFAULT_POOL_NUM 1

typedef bool (*match)(void *item, void *input_arg);

template <class T>
class MemPool
{
public:
  MemPool(const char *tag) : name(tag)
  {
    this->size = 0;

    pthread_mutexattr_t mutexatr;
    pthread_mutexattr_init(&mutexatr);
    pthread_mutexattr_settype(&mutexatr, PTHREAD_MUTEX_RECURSIVE);

    MUTEX_INIT(&mutex, &mutexatr);
  }

  virtual ~MemPool()
  {
    MUTEX_DESTROY(&mutex);
  }

  /**
   * init memory pool, the major job is to alloc memory for memory pool
   * @param pool_num, memory pool's number
   * @param item_num_per_pool, how many items per pool.
   * @return
   */
  virtual int init(
      bool dynamic = true, int pool_num = DEFAULT_POOL_NUM, int item_num_per_pool = DEFAULT_ITEM_NUM_PER_POOL) = 0;

  /**
   * Do cleanup job for memory pool
   */
  virtual void cleanup() = 0;

  /**
   * If dynamic has been set, extend current memory pool,
   */
  virtual int extend() = 0;

  /**
   * Alloc one frame from memory Pool
   * @return
   */
  virtual T *alloc() = 0;

  /**
   * Free one item, the resouce will return to memory Pool
   * @param item
   */
  virtual void free(T *item) = 0;

  /**
   * Print the MemPool status
   * @return
   */
  virtual std::string to_string() = 0;

  const std::string get_name() const
  {
    return name;
  }
  bool is_dynamic() const
  {
    return dynamic;
  }
  int get_size() const
  {
    return size;
  }

protected:
  pthread_mutex_t mutex;
  int size;
  bool dynamic;
  std::string name;
};

/**
 * MemoryPoolSimple is a simple Memory Pool manager
 * The objects is constructed when creating the pool and destructed when the pool is cleanup.
 * `alloc` calls T's `reinit` routine and `free` calls T's `reset`
 */
template <class T>
class MemPoolSimple : public MemPool<T>
{
public:
  MemPoolSimple(const char *tag) : MemPool<T>(tag)
  {}

  virtual ~MemPoolSimple()
  {
    cleanup();
  }

  /**
   * init memory pool, the major job is to alloc memory for memory pool
   * @param pool_num, memory pool's number
   * @param item_num_per_pool, how many items per pool.
   * @return 0 for success and others failure
   */
  int init(bool dynamic = true, int pool_num = DEFAULT_POOL_NUM, int item_num_per_pool = DEFAULT_ITEM_NUM_PER_POOL);

  /**
   * Do cleanup job for memory pool
   */
  void cleanup();

  /**
   * If dynamic has been set, extend current memory pool,
   */
  int extend();

  /**
   * Alloc one frame from memory Pool
   * @return
   */
  T *alloc();

  /**
   * Free one item, the resouce will return to memory Pool
   * @param item
   */
  void free(T *item);

  /**
   * Print the MemPool status
   * @return
   */
  std::string to_string();

  int get_item_num_per_pool() const
  {
    return item_num_per_pool;
  }

  int get_used_num()
  {
    MUTEX_LOCK(&this->mutex);
    auto num = used.size();
    MUTEX_UNLOCK(&this->mutex);
    return num;
  }

protected:
  std::list<T *> pools;
  std::set<T *> used;
  std::list<T *> frees;
  int item_num_per_pool;
};

template <class T>
int MemPoolSimple<T>::init(bool dynamic, int pool_num, int item_num_per_pool)
{
  if (pools.empty() == false) {
    LOG_WARN("Memory pool has been initialized, but still begin to be initialized, this->name:%s.", this->name.c_str());
    return 0;
  }

  if (pool_num <= 0 || item_num_per_pool <= 0) {
    LOG_ERROR("Invalid arguments,  pool_num:%d, item_num_per_pool:%d, this->name:%s.",
              pool_num, item_num_per_pool, this->name.c_str());
    return -1;
  }

  this->item_num_per_pool = item_num_per_pool;
  // in order to init memory pool, enable dynamic here
  this->dynamic = true;
  for (int i = 0; i < pool_num; i++) {
    if (extend() < 0) {
      cleanup();
      return -1;
    }
  }
  this->dynamic = dynamic;

  LOG_INFO("Extend one pool, this->size:%d, item_num_per_pool:%d, this->name:%s.",
           this->size, item_num_per_pool, this->name.c_str());
  return 0;
}

template <class T>
void MemPoolSimple<T>::cleanup()
{
  if (pools.empty() == true) {
    LOG_WARN("Begin to do cleanup, but there is no memory pool, this->name:%s!", this->name.c_str());
    return;
  }

  MUTEX_LOCK(&this->mutex);

  used.clear();
  frees.clear();
  this->size = 0;

  for (typename std::list<T *>::iterator iter = pools.begin(); iter != pools.end(); iter++) {
    T *pool = *iter;

    delete[] pool;
  }
  pools.clear();
  MUTEX_UNLOCK(&this->mutex);
  LOG_INFO("Successfully do cleanup, this->name:%s.", this->name.c_str());
}

template <class T>
int MemPoolSimple<T>::extend()
{
  if (this->dynamic == false) {
    LOG_ERROR("Disable dynamic extend memory pool, but begin to extend, this->name:%s", this->name.c_str());
    return -1;
  }

  MUTEX_LOCK(&this->mutex);
  T *pool = new T[item_num_per_pool];
  if (pool == nullptr) {
    MUTEX_UNLOCK(&this->mutex);
    LOG_ERROR("Failed to extend memory pool, this->size:%d, item_num_per_pool:%d, this->name:%s.",
              this->size, item_num_per_pool, this->name.c_str());
    return -1;
  }

  pools.push_back(pool);
  this->size += item_num_per_pool;
  for (int i = 0; i < item_num_per_pool; i++) {
    frees.push_back(pool + i);
  }
  MUTEX_UNLOCK(&this->mutex);

  LOG_INFO("Extend one pool, this->size:%d, item_num_per_pool:%d, this->name:%s.",
           this->size, item_num_per_pool, this->name.c_str());
  return 0;
}

template <class T>
T *MemPoolSimple<T>::alloc()
{
  MUTEX_LOCK(&this->mutex);
  if (frees.empty() == true) {
    if (this->dynamic == false) {
      MUTEX_UNLOCK(&this->mutex);
      return nullptr;
    }

    if (extend() < 0) {
      MUTEX_UNLOCK(&this->mutex);
      return nullptr;
    }
  }
  T *buffer = frees.front();
  frees.pop_front();

  used.insert(buffer);

  MUTEX_UNLOCK(&this->mutex);
  buffer->reinit();
  return buffer;
}

template <class T>
void MemPoolSimple<T>::free(T *buf)
{
  buf->reset();
  
  MUTEX_LOCK(&this->mutex);

  size_t num = used.erase(buf);
  if (num == 0) {
    MUTEX_UNLOCK(&this->mutex);
    LOG_WARN("No entry of %p in %s.", buf, this->name.c_str());
    print_stacktrace();
    return;
  }

  frees.push_back(buf);

  MUTEX_UNLOCK(&this->mutex);
  return;  // TODO for test
}

template <class T>
std::string MemPoolSimple<T>::to_string()
{
  std::stringstream ss;

  ss << "name:" << this->name << ","
     << "dyanmic:" << this->dynamic << ","
     << "size:" << this->size << ","
     << "pool_size:" << this->pools.size() << ","
     << "used_size:" << this->used.size() << ","
     << "free_size:" << this->frees.size();
  return ss.str();
}

class MemPoolItem
{
public:
  using unique_ptr = std::unique_ptr<void, std::function<void(void * const)>>;

public:
  MemPoolItem(const char *tag) : name(tag)
  {
    this->size = 0;

    pthread_mutexattr_t mutexatr;
    pthread_mutexattr_init(&mutexatr);
    pthread_mutexattr_settype(&mutexatr, PTHREAD_MUTEX_RECURSIVE);

    MUTEX_INIT(&mutex, &mutexatr);
  }

  virtual ~MemPoolItem()
  {
    cleanup();
    MUTEX_DESTROY(&mutex);
  }

  /**
   * init memory pool, the major job is to alloc memory for memory pool
   * @param pool_num, memory pool's number
   * @param item_num_per_pool, how many items per pool.
   * @return
   */
  int init(int item_size, bool dynamic = true, int pool_num = DEFAULT_POOL_NUM,
      int item_num_per_pool = DEFAULT_ITEM_NUM_PER_POOL);

  /**
   * Do cleanup job for memory pool
   */
  void cleanup();

  /**
   * If dynamic has been set, extend current memory pool,
   */
  int extend();

  /**
   * Alloc one frame from memory Pool
   * @return
   */
  void *alloc();
  unique_ptr alloc_unique_ptr();

  /**
   * Free one item, the resouce will return to memory Pool
   * @param item
   */
  void free(void *item);

  /**
   * Check whether this item has been used before.
   * @param item
   * @return
   */
  bool is_used(void *item)
  {
    MUTEX_LOCK(&mutex);
    auto it = used.find(item);
    MUTEX_UNLOCK(&mutex);
    return it != used.end();
  }

  std::string to_string()
  {

    std::stringstream ss;

    ss << "name:" << this->name << ","
       << "dyanmic:" << this->dynamic << ","
       << "size:" << this->size << ","
       << "pool_size:" << this->pools.size() << ","
       << "used_size:" << this->used.size() << ","
       << "free_size:" << this->frees.size();
    return ss.str();
  }

  const std::string get_name() const
  {
    return name;
  }
  bool is_dynamic() const
  {
    return dynamic;
  }
  int get_size() const
  {
    return size;
  }
  int get_item_size() const
  {
    return item_size;
  }
  int get_item_num_per_pool() const
  {
    return item_num_per_pool;
  }

  int get_used_num()
  {
    MUTEX_LOCK(&mutex);
    auto num = used.size();
    MUTEX_UNLOCK(&mutex);
    return num;
  }

protected:
  pthread_mutex_t mutex;
  std::string name;
  bool dynamic;
  int size;
  int item_size;
  int item_num_per_pool;

  std::list<void *> pools;
  std::set<void *> used;
  std::list<void *> frees;
};

}  // namespace common
