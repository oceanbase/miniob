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

#pragma once

#include "common/rc.h"
#include "common/lang/deque.h"
#include "common/lang/vector.h"
#include "storage/buffer/page.h"

class Frame;
class DiskBufferPool;

namespace common {
class SharedMutex;
}

/**
 * @brief 对指定页面做的操作类型
 */
enum class LatchMemoType
{
  NONE,       /// 什么都不做
  SHARED,     /// 共享锁
  EXCLUSIVE,  /// 独占锁
  PIN,        /// pin住页面，增加引用计数
};

struct LatchMemoItem
{
  LatchMemoItem() = default;
  LatchMemoItem(LatchMemoType type, Frame *frame);
  LatchMemoItem(LatchMemoType type, common::SharedMutex *lock);

  LatchMemoType        type  = LatchMemoType::NONE;
  Frame               *frame = nullptr;
  common::SharedMutex *lock  = nullptr;
};

class LatchMemo final
{
public:
  /**
   * @brief 当前遇到的场景都是针对单个BufferPool的，不过从概念上讲，不一定做这个限制
   */
  LatchMemo(DiskBufferPool *buffer_pool);
  ~LatchMemo();

  RC get_page(PageNum page_num, Frame *&frame);

  /// @brief 分配页面
  RC allocate_page(Frame *&frame);

  /// @brief 标记为即将释放的页面
  void dispose_page(PageNum page_num);

  /// @brief 对指定页面加锁
  void latch(Frame *frame, LatchMemoType type);
  void xlatch(Frame *frame);
  void slatch(Frame *frame);
  bool try_slatch(Frame *frame);

  void xlatch(common::SharedMutex *lock);
  void slatch(common::SharedMutex *lock);

  void release();

  void release_to(int point);

  int memo_point() const { return static_cast<int>(items_.size()); }

private:
  void release_item(LatchMemoItem &item);

private:
  DiskBufferPool      *buffer_pool_ = nullptr;
  deque<LatchMemoItem> items_;
  vector<PageNum>      disposed_pages_;  /// 等待释放的页面
};