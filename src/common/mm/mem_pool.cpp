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
// Created by Longda on 2022/1/28.
//

#include "common/mm/mem_pool.h"
namespace common {

int MemPoolItem::init(int item_size, bool dynamic, int pool_num, int item_num_per_pool)
{
  if (pools.empty() == false) {
    LOG_WARN("Memory pool has been initialized, but still begin to be initialized, this->name:%s.", this->name.c_str());
    return 0;
  }

  if (item_size <= 0 || pool_num <= 0 || item_num_per_pool <= 0) {
    LOG_ERROR("Invalid arguments, item_size:%d, pool_num:%d, item_num_per_pool:%d, this->name:%s.",
        item_size, pool_num, item_num_per_pool, this->name.c_str());
    return -1;
  }

  this->item_size         = item_size;
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

  LOG_INFO("Extend one pool, this->size:%d, item_size:%d, item_num_per_pool:%d, this->name:%s.",
      this->size, item_size, item_num_per_pool, this->name.c_str());
  return 0;
}

void MemPoolItem::cleanup()
{
  if (pools.empty() == true) {
    LOG_WARN("Begin to do cleanup, but there is no memory pool, this->name:%s!", this->name.c_str());
    return;
  }

  MUTEX_LOCK(&this->mutex);

  used.clear();
  frees.clear();
  this->size = 0;

  for (list<void *>::iterator iter = pools.begin(); iter != pools.end(); iter++) {
    void *pool = *iter;

    ::free(pool);
  }
  pools.clear();
  MUTEX_UNLOCK(&this->mutex);
  LOG_INFO("Successfully do cleanup, this->name:%s.", this->name.c_str());
}

int MemPoolItem::extend()
{
  if (this->dynamic == false) {
    LOG_ERROR("Disable dynamic extend memory pool, but begin to extend, this->name:%s", this->name.c_str());
    return -1;
  }

  MUTEX_LOCK(&this->mutex);
  void *pool = malloc(static_cast<size_t>(item_num_per_pool) * item_size);
  if (pool == nullptr) {
    MUTEX_UNLOCK(&this->mutex);
    LOG_ERROR("Failed to extend memory pool, this->size:%d, item_num_per_pool:%d, this->name:%s.",
        this->size,
        item_num_per_pool,
        this->name.c_str());
    return -1;
  }

  pools.push_back(pool);
  this->size += item_num_per_pool;
  for (int i = 0; i < item_num_per_pool; i++) {
    char *item = (char *)pool + i * item_size;
    frees.push_back((void *)item);
  }
  MUTEX_UNLOCK(&this->mutex);

  LOG_INFO("Extend one pool, this->size:%d, item_size:%d, item_num_per_pool:%d, this->name:%s.",
      this->size,
      item_size,
      item_num_per_pool,
      this->name.c_str());
  return 0;
}

void *MemPoolItem::alloc()
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
  void *buffer = frees.front();
  frees.pop_front();

  used.insert(buffer);

  MUTEX_UNLOCK(&this->mutex);

  memset(buffer, 0, sizeof(item_size));
  return buffer;
}

MemPoolItem::item_unique_ptr MemPoolItem::alloc_unique_ptr()
{
  void *item    = this->alloc();
  auto  deleter = [this](void *p) { this->free(p); };
  return MemPoolItem::item_unique_ptr(item, deleter);
}

void MemPoolItem::free(void *buf)
{
  MUTEX_LOCK(&this->mutex);

  size_t num = used.erase(buf);
  if (num == 0) {
    MUTEX_UNLOCK(&this->mutex);
    LOG_WARN("No entry of %p in %s.", buf, this->name.c_str());
    return;
  }

  frees.push_back(buf);

  MUTEX_UNLOCK(&this->mutex);
  return;  // TODO for test
}
}  // namespace common