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
// Created by lianyu on 2022/10/29.
//

#pragma once

#include <pthread.h>
#include <string.h>

#include "common/lang/mutex.h"
#include "common/lang/string.h"
#include "common/lang/atomic.h"
#include "common/lang/unordered_map.h"
#include "common/log/log.h"
#include "common/types.h"
#include "storage/buffer/page.h"

/**
 * @brief 页帧标识符
 * @ingroup BufferPool
 */
class FrameId
{
public:
  FrameId() = default;
  FrameId(int buffer_pool_id, PageNum page_num);
  bool    equal_to(const FrameId &other) const;
  bool    operator==(const FrameId &other) const;
  size_t  hash() const;
  int     buffer_pool_id() const;
  PageNum page_num() const;

  void set_buffer_pool_id(int buffer_pool_id) { buffer_pool_id_ = buffer_pool_id; }
  void set_page_num(PageNum page_num) { page_num_ = page_num; }

  string to_string() const;

private:
  int     buffer_pool_id_ = -1;
  PageNum page_num_       = -1;
};

/**
 * @brief 页帧
 * @ingroup BufferPool
 * @details 页帧是磁盘文件在内存中的表示。磁盘文件按照页面来操作，操作之前先映射到内存中，
 * 将磁盘数据读取到内存中，也就是页帧。
 *
 * 当某个页面被淘汰时，如果有些内容曾经变更过，那么就需要将这些内容刷新到磁盘上。这里有
 * 一个dirty标识，用来标识页面是否被修改过。
 *
 * 为了防止在使用过程中页面被淘汰，这里使用了pin count，当页面被使用时，pin count会增加，
 * 当页面不再使用时，pin count会减少。当pin count为0时，页面可以被淘汰。
 */
class Frame
{
public:
  ~Frame()
  {
    // LOG_DEBUG("deallocate frame. this=%p, lbt=%s", this, common::lbt());
  }

  /**
   * @brief reinit 和 reset 在 MemPoolSimple 中使用
   * @details 在 MemPoolSimple 分配和释放一个Frame对象时，不会调用构造函数和析构函数，
   * 而是调用reinit和reset。
   */
  void reinit() {}
  void reset() {}

  void clear_page() { memset(&page_, 0, sizeof(page_)); }

  int  buffer_pool_id() const { return frame_id_.buffer_pool_id(); }
  void set_buffer_pool_id(int id) { frame_id_.set_buffer_pool_id(id); }

  /**
   * @brief 在磁盘和内存中内容完全一致的数据页
   * @details 磁盘文件划分为一个个页面，每次从磁盘加载到内存中，也是一个页面，就是 Page。
   * frame 是为了管理这些页面而维护的一个数据结构。
   */
  Page &page() { return page_; }

  /**
   * @brief 每个页面都有一个编号
   * @details 当前页面编号记录在了页面数据中，其实可以不记录，从磁盘中加载时记录在Frame信息中即可。
   */
  PageNum page_num() const { return frame_id_.page_num(); }
  void    set_page_num(PageNum page_num) { frame_id_.set_page_num(page_num); }
  FrameId frame_id() const { return frame_id_; }

  /**
   * @brief 为了实现持久化，需要将页面的修改记录记录到日志中，这里记录了日志序列号
   * @details 如果当前页面从磁盘中加载出来时，它的日志序列号比当前WAL(Write-Ahead-Logging)中的一些
   * 序列号要小，那就可以从日志中读取这些更大序列号的日志，做重做操作，将页面恢复到最新状态，也就是redo。
   */
  LSN  lsn() const { return page_.lsn; }
  void set_lsn(LSN lsn) { page_.lsn = lsn; }

  /**
   * @brief 页面校验和
   * @details 用于校验页面完整性。如果页面写入一半时出现异常，可以通过校验和检测出来。
   */
  CheckSum check_sum() const { return page_.check_sum; }
  void     set_check_sum(CheckSum check_sum) { page_.check_sum = check_sum; }

  /**
   * @brief 刷新当前内存页面的访问时间
   * @details 由于内存是有限的，比磁盘要小很多。那当我们访问某些文件页面时，可能由于内存不足
   * 而要淘汰一些页面。我们选择淘汰哪些页面呢？这里使用了LRU算法，即最近最少使用的页面被淘汰。
   * 最近最少使用，采用的依据就是访问时间。所以每次访问某个页面时，我们都要刷新一下访问时间。
   */
  void access();

  /**
   * @brief 标记指定页面为“脏”页。
   * @details 如果修改了页面的内容，则应调用此函数，
   * 以便该页面被淘汰出缓冲区时系统将新的页面数据写入磁盘文件
   */
  void mark_dirty() { dirty_ = true; }

  /**
   * @brief 重置“脏”标记
   * @details 如果页面已经被写入磁盘文件，则应调用此函数。
   */
  void clear_dirty() { dirty_ = false; }
  bool dirty() const { return dirty_; }

  char *data() { return page_.data; }

  bool can_purge() { return pin_count_.load() == 0; }

  /**
   * @brief 给当前页帧增加引用计数
   * pin通常都会加着frame manager锁来访问。
   * 当我们访问某个页面时，我们不期望此页面被淘汰，所以我们会增加引用计数。
   */
  void pin();

  /**
   * @brief 释放一个当前页帧的引用计数
   * 与pin对应，但是通常不会加着frame manager的锁来访问
   */
  int unpin();
  int pin_count() const { return pin_count_.load(); }

  void write_latch();
  void write_latch(intptr_t xid);

  void write_unlatch();
  void write_unlatch(intptr_t xid);

  void read_latch();
  void read_latch(intptr_t xid);
  bool try_read_latch();

  void read_unlatch();
  void read_unlatch(intptr_t xid);

  string to_string() const;

private:
  friend class BufferPool;

  bool          dirty_ = false;
  atomic<int>   pin_count_{0};
  unsigned long acc_time_ = 0;
  FrameId       frame_id_;
  Page          page_;

  /// 在非并发编译时，加锁解锁动作将什么都不做
  common::RecursiveSharedMutex lock_;

  /// 使用一些手段来做测试，提前检测出头疼的死锁问题
  /// 如果编译时没有增加调试选项，这些代码什么都不做
  common::DebugMutex           debug_lock_;
  intptr_t                     write_locker_          = 0;
  int                          write_recursive_count_ = 0;
  unordered_map<intptr_t, int> read_lockers_;
};
