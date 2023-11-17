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
// Created by Wangyunlai on 2023/03/08.
//

#include "storage/trx/latch_memo.h"
#include "common/lang/mutex.h"
#include "storage/buffer/disk_buffer_pool.h"
#include "storage/buffer/frame.h"

LatchMemoItem::LatchMemoItem(LatchMemoType type, Frame *frame)
{
  this->type  = type;
  this->frame = frame;
}

LatchMemoItem::LatchMemoItem(LatchMemoType type, common::SharedMutex *lock)
{
  this->type = type;
  this->lock = lock;
}

////////////////////////////////////////////////////////////////////////////////

LatchMemo::LatchMemo(DiskBufferPool *buffer_pool) : buffer_pool_(buffer_pool) {}

LatchMemo::~LatchMemo() { this->release(); }

RC LatchMemo::get_page(PageNum page_num, Frame *&frame)
{
  frame = nullptr;

  RC rc = buffer_pool_->get_this_page(page_num, &frame);
  if (rc != RC::SUCCESS) {
    return rc;
  }

  items_.emplace_back(LatchMemoType::PIN, frame);
  return RC::SUCCESS;
}

RC LatchMemo::allocate_page(Frame *&frame)
{
  frame = nullptr;

  RC rc = buffer_pool_->allocate_page(&frame);
  if (rc == RC::SUCCESS) {
    items_.emplace_back(LatchMemoType::PIN, frame);
    ASSERT(frame->pin_count() == 1, "allocate a new frame. frame=%s", to_string(*frame).c_str());
  }

  return rc;
}

void LatchMemo::dispose_page(PageNum page_num) { disposed_pages_.emplace_back(page_num); }

void LatchMemo::latch(Frame *frame, LatchMemoType type)
{
  switch (type) {
    case LatchMemoType::EXCLUSIVE: {
      frame->write_latch();
    } break;
    case LatchMemoType::SHARED: {
      frame->read_latch();
    } break;
    default: {
      ASSERT(false, "invalid latch type: %d", static_cast<int>(type));
    }
  }

  items_.emplace_back(type, frame);
}

void LatchMemo::xlatch(Frame *frame) { this->latch(frame, LatchMemoType::EXCLUSIVE); }

void LatchMemo::slatch(Frame *frame) { this->latch(frame, LatchMemoType::SHARED); }

bool LatchMemo::try_slatch(Frame *frame)
{
  bool ret = frame->try_read_latch();
  if (ret) {
    items_.emplace_back(LatchMemoType::SHARED, frame);
  }
  return ret;
}

void LatchMemo::xlatch(common::SharedMutex *lock)
{
  lock->lock();
  items_.emplace_back(LatchMemoType::EXCLUSIVE, lock);
  LOG_DEBUG("lock root success");
}

void LatchMemo::slatch(common::SharedMutex *lock)
{
  lock->lock_shared();
  items_.emplace_back(LatchMemoType::SHARED, lock);
}

void LatchMemo::release_item(LatchMemoItem &item)
{
  switch (item.type) {
    case LatchMemoType::EXCLUSIVE: {
      if (item.frame != nullptr) {
        item.frame->write_unlatch();
      } else {
        LOG_DEBUG("release root lock");
        item.lock->unlock();
      }
    } break;
    case LatchMemoType::SHARED: {
      if (item.frame != nullptr) {
        item.frame->read_unlatch();
      } else {
        item.lock->unlock_shared();
      }
    } break;
    case LatchMemoType::PIN: {
      buffer_pool_->unpin_page(item.frame);
    } break;

    default: {
      ASSERT(false, "invalid latch type: %d", static_cast<int>(item.type));
    }
  }
}

void LatchMemo::release()
{
  int point = static_cast<int>(items_.size());
  release_to(point);

  for (PageNum page_num : disposed_pages_) {
    buffer_pool_->dispose_page(page_num);
  }
  disposed_pages_.clear();
}

void LatchMemo::release_to(int point)
{
  ASSERT(point >= 0 && point <= static_cast<int>(items_.size()), 
         "invalid memo point. point=%d, items size=%d",
         point, static_cast<int>(items_.size()));

  auto iter = items_.begin();
  for (int i = point - 1; i >= 0; i--, ++iter) {
    LatchMemoItem &item = items_[i];
    release_item(item);
  }
  items_.erase(items_.begin(), iter);
}
