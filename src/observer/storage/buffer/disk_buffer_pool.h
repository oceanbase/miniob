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
// Created by Meiyi & Longda on 2021/4/13.
//
#pragma once

#include <fcntl.h>
#include <functional>
#include <mutex>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <optional>

#include "common/lang/bitmap.h"
#include "common/lang/lru_cache.h"
#include "common/lang/mutex.h"
#include "common/lang/memory.h"
#include "common/lang/unordered_map.h"
#include "common/mm/mem_pool.h"
#include "common/rc.h"
#include "common/types.h"
#include "storage/buffer/frame.h"
#include "storage/buffer/page.h"
#include "storage/buffer/buffer_pool_log.h"

class BufferPoolManager;
class DiskBufferPool;
class DoubleWriteBuffer;
class LogHandler;
class BufferPoolLogHandler;

/**
 * @brief BufferPool 的实现
 * @defgroup BufferPool
 */

#define BP_FILE_SUB_HDR_SIZE (sizeof(BPFileSubHeader))

/**
 * @brief BufferPool的文件第一个页面，存放一些元数据信息，包括了后面每页的分配信息。
 * @ingroup BufferPool
 * @details
 * @code
 * TODO 1. 当前的做法，只能分配比较少的页面，你可以扩展一下，支持更多的页面或无限多的页面吗？
 *         可以参考Linux ext(n)和Windows NTFS等文件系统
 *      2. 当前使用bitmap存放页面分配情况，但是这种方法在页面非常多的时候，查找空闲页面的
 *         效率非常低，你有办法优化吗？
 * @endcode
 */
struct BPFileHeader
{
  int32_t buffer_pool_id;   //! buffer pool id
  int32_t page_count;       //! 当前文件一共有多少个页面
  int32_t allocated_pages;  //! 已经分配了多少个页面
  char    bitmap[0];        //! 页面分配位图, 第0个页面(就是当前页面)，总是1

  /**
   * 能够分配的最大的页面个数，即bitmap的字节数 乘以8
   */
  static const int MAX_PAGE_NUM = (BP_PAGE_DATA_SIZE - sizeof(page_count) - sizeof(allocated_pages)) * 8;

  string to_string() const;
};

/**
 * @brief 管理页面Frame
 * @ingroup BufferPool
 * @details 管理内存中的页帧。内存是有限的，内存中能够存放的页帧个数也是有限的。
 * 当内存中的页帧不够用时，需要从内存中淘汰一些页帧，以便为新的页帧腾出空间。
 * 这个管理器负责为所有的BufferPool提供页帧管理服务，也就是所有的BufferPool磁盘文件
 * 在访问时都使用这个管理器映射到内存。
 */
class BPFrameManager
{
public:
  BPFrameManager(const char *tag);

  RC init(int pool_num);
  RC cleanup();

  /**
   * @brief 获取指定的页面
   *
   * @param buffer_pool_id buffer Pool标识
   * @param page_num  页面号
   * @return Frame* 页帧指针
   */
  Frame *get(int buffer_pool_id, PageNum page_num);

  /**
   * @brief 列出所有指定文件的页面
   *
   * @param buffer_pool_id buffer Pool标识
   * @return list<Frame *> 页帧列表
   */
  list<Frame *> find_list(int buffer_pool_id);

  /**
   * @brief 分配一个新的页面
   *
   * @param buffer_pool_id buffer Pool标识
   * @param page_num 页面编号
   * @return Frame* 页帧指针
   */
  Frame *alloc(int buffer_pool_id, PageNum page_num);

  /**
   * 尽管frame中已经包含了buffer_pool_id和page_num，但是依然要求
   * 传入，因为frame可能忘记初始化或者没有初始化
   */
  RC free(int buffer_pool_id, PageNum page_num, Frame *frame);

  /**
   * 如果不能从空闲链表中分配新的页面，就使用这个接口，
   * 尝试从pin count=0的页面中淘汰一些
   * @param count 想要purge多少个页面
   * @param purger 需要在释放frame之前，对页面做些什么操作。当前是刷新脏数据到磁盘
   * @return 返回本次清理了多少个页面
   */
  int purge_frames(int count, function<RC(Frame *frame)> purger);

  size_t frame_num() const { return frames_.count(); }

  /**
   * 测试使用。返回已经从内存申请的个数
   */
  size_t total_frame_num() const { return allocator_.get_size(); }

private:
  Frame *get_internal(const FrameId &frame_id);
  RC     free_internal(const FrameId &frame_id, Frame *frame);

private:
  class BPFrameIdHasher
  {
  public:
    size_t operator()(const FrameId &frame_id) const { return frame_id.hash(); }
  };

  using FrameLruCache  = common::LruCache<FrameId, Frame *, BPFrameIdHasher>;
  using FrameAllocator = common::MemPoolSimple<Frame>;

  mutex          lock_;
  FrameLruCache  frames_;
  FrameAllocator allocator_;
};

/**
 * @brief 用于遍历BufferPool中的所有页面
 * @ingroup BufferPool
 */
class BufferPoolIterator
{
public:
  BufferPoolIterator();
  ~BufferPoolIterator();

  RC      init(DiskBufferPool &bp, PageNum start_page = 0);
  bool    has_next();
  PageNum next();
  RC      reset();

private:
  common::Bitmap bitmap_;
  PageNum        current_page_num_ = -1;
};

/**
 * @brief BufferPool的实现
 * @ingroup BufferPool
 * @details 一个文件被划分成多个相同大小的页面，并在需要访问的时候，会从文件读取到内存中。
 * DiskBufferPool 就负责管理磁盘文件，以及负责管理页面在文件与内存中的交互，比如读取、写回。
 */
class DiskBufferPool final
{
public:
  DiskBufferPool(BufferPoolManager &bp_manager, BPFrameManager &frame_manager, DoubleWriteBuffer &dblwr_manager,
      LogHandler &log_handler);
  ~DiskBufferPool();

  /**
   * 根据文件名打开一个分页文件
   */
  RC open_file(const char *file_name);

  /**
   * 关闭分页文件
   */
  RC close_file();

  /**
   * 根据文件ID和页号获取指定页面到缓冲区，返回页面句柄指针。
   */
  RC get_this_page(PageNum page_num, Frame **frame);

  /**
   * @brief 在指定文件中分配一个新的页面，并将其放入缓冲区，返回页面句柄指针。
   * @details 分配页面时，如果文件中有空闲页，就直接分配一个空闲页；
   * 如果文件中没有空闲页，则扩展文件规模来增加新的空闲页。
   */
  RC allocate_page(Frame **frame);

  /**
   * @brief 释放某个页面，将此页面设置为未分配状态
   *
   * @param page_num 待释放的页面
   */
  RC dispose_page(PageNum page_num);

  /**
   * @brief 释放指定文件关联的页的内存
   * 如果已经脏， 则刷到磁盘，除了pinned page
   */
  RC purge_page(PageNum page_num);
  RC purge_all_pages();

  /**
   * @brief 用于解除pageHandle对应页面的驻留缓冲区限制
   *
   * 在调用GetThisPage或AllocatePage函数将一个页面读入缓冲区后，
   * 该页面被设置为驻留缓冲区状态，以防止其在处理过程中被置换出去，
   * 因此在该页面使用完之后应调用此函数解除该限制，使得该页面此后可以正常地被淘汰出缓冲区
   */
  RC unpin_page(Frame *frame);

  /**
   * 检查是否所有页面都是pin count == 0状态(除了第1个页面)
   * 调试使用
   */
  RC check_all_pages_unpinned();

  int file_desc() const;

  /**
   * 如果页面是脏的，就将数据刷新到double write buffer
   */
  RC flush_page(Frame &frame);

  /**
   * 刷新所有页面到double write buffer，即使pin count不是0
   */
  RC flush_all_pages();

  /**
   * 回放日志时处理page0中已被认定为不存在的page
   */
  RC recover_page(PageNum page_num);

  /**
   * 刷新页面到磁盘
   */
  RC write_page(PageNum page_num, Page &page);

  RC redo_allocate_page(LSN lsn, PageNum page_num);
  RC redo_deallocate_page(LSN lsn, PageNum page_num);

public:
  int32_t id() const { return buffer_pool_id_; }

  const char *filename() const { return file_name_.c_str(); }

protected:
  RC allocate_frame(PageNum page_num, Frame **buf);

  /**
   * 刷新指定页面到磁盘(flush)，并且释放关联的Frame
   */
  RC purge_frame(PageNum page_num, Frame *used_frame);
  RC check_page_num(PageNum page_num);

  /**
   * 加载指定页面的数据到内存中
   */
  RC load_page(PageNum page_num, Frame *frame);

  /**
   * 如果页面是脏的，就将数据刷新到磁盘
   */
  RC flush_page_internal(Frame &frame);

private:
  BufferPoolManager   &bp_manager_;     /// BufferPool 管理器
  BPFrameManager      &frame_manager_;  /// Frame 管理器
  DoubleWriteBuffer   &dblwr_manager_;  /// Double Write Buffer 管理器
  BufferPoolLogHandler log_handler_;    /// BufferPool 日志处理器

  int file_desc_ = -1;  /// 文件描述符
  /// 由于在最开始打开文件时，没有正确的buffer pool id不能加载header frame，所以单独从文件中读取此标识
  int32_t       buffer_pool_id_ = -1;
  Frame        *hdr_frame_      = nullptr;  /// 文件头页面
  BPFileHeader *file_header_    = nullptr;  /// 文件头
  set<PageNum>  disposed_pages_;            /// 已经释放的页面

  string file_name_;  /// 文件名

  common::Mutex lock_;
  common::Mutex wr_lock_;

private:
  friend class BufferPoolIterator;
};

/**
 * @brief BufferPool的管理类
 * @ingroup BufferPool
 */
class BufferPoolManager final
{
public:
  BufferPoolManager(int memory_size = 0);
  ~BufferPoolManager();

  RC init(unique_ptr<DoubleWriteBuffer> dblwr_buffer);

  RC create_file(const char *file_name);
  RC open_file(LogHandler &log_handler, const char *file_name, DiskBufferPool *&bp);
  RC close_file(const char *file_name);

  RC flush_page(Frame &frame);

  BPFrameManager    &get_frame_manager() { return frame_manager_; }
  DoubleWriteBuffer *get_dblwr_buffer() { return dblwr_buffer_.get(); }

  /**
   * @brief 根据ID获取对应的BufferPool对象
   * @details 在做redo时，需要根据ID获取对应的BufferPool对象，然后让bufferPool对象自己做redo
   * @param id buffer pool id
   * @param bp buffer pool 对象
   */
  RC get_buffer_pool(int32_t id, DiskBufferPool *&bp);

private:
  BPFrameManager frame_manager_{"BufPool"};

  unique_ptr<DoubleWriteBuffer> dblwr_buffer_;

  common::Mutex                            lock_;
  unordered_map<string, DiskBufferPool *>  buffer_pools_;
  unordered_map<int32_t, DiskBufferPool *> id_to_buffer_pools_;
  atomic<int32_t> next_buffer_pool_id_{1};  // 系统启动时，会打开所有的表，这样就可以知道当前系统最大的ID是多少了
};
