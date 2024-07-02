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
// Created by Wenbin1002 on 2024/04/16
//

#pragma once

#include "common/lang/mutex.h"
#include "common/lang/unordered_map.h"
#include "common/types.h"
#include "common/rc.h"
#include "storage/buffer/page.h"

class DiskBufferPool;
struct DoubleWritePage;
class BufferPoolManager;

class DoubleWriteBuffer
{
public:
  DoubleWriteBuffer()          = default;
  virtual ~DoubleWriteBuffer() = default;

  /**
   * 将页面加入buffer，并且写入磁盘中的共享表空间
   */
  virtual RC add_page(DiskBufferPool *bp, PageNum page_num, Page &page) = 0;

  virtual RC read_page(DiskBufferPool *bp, PageNum page_num, Page &page) = 0;

  /**
   * @brief 清空所有与指定buffer pool关联的页面
   */
  virtual RC clear_pages(DiskBufferPool *bp) = 0;
};

struct DoubleWriteBufferHeader
{
  int32_t page_cnt = 0;

  static const int32_t SIZE;
};

// TODO change to FrameId
struct DoubleWritePageKey
{
  int32_t buffer_pool_id;
  PageNum page_num;

  bool operator==(const DoubleWritePageKey &other) const
  {
    return buffer_pool_id == other.buffer_pool_id && page_num == other.page_num;
  }
};

struct DoubleWritePageKeyHash
{
  size_t operator()(const DoubleWritePageKey &key) const
  {
    return std::hash<int32_t>()(key.buffer_pool_id) ^ std::hash<PageNum>()(key.page_num);
  }
};

/**
 * @brief 页面二次缓冲区，为了解决页面原子写入的问题
 * @ingroup BufferPool
 * @details 一个页面通常比较大，不能保证要么都写入磁盘成功，要么不写入磁盘。如果存在写入一部分的情况，
 * 我们应该有手段检测出来，否则会遇到灾难性的数据不一致问题。
 * 这里的解决方案是在我们写入真实页面数据之前，先将数据放入一个公共的缓冲区，也就是DoubleWriteBuffer，
 * DoubleWriteBuffer会先在一个共享磁盘文件中写入页面数据，在确定写入成功后，再写入真实的页面。
 * 当我们从磁盘中读取页面时，会校验页面的checksum，如果校验失败，则说明页面写入不完整，这时候可以从
 * DoubleWriteBuffer中读取数据。
 *
 * @note 每次都要保证，不管在内存中还是在文件中，这里的数据都是最新的，都比Buffer pool中的数据要新
 */
class DiskDoubleWriteBuffer : public DoubleWriteBuffer
{
public:
  /**
   * @brief 构造函数
   *
   * @param bp_manager 关联的buffer pool manager
   * @param max_pages  内存中保存的最大页面数
   */
  DiskDoubleWriteBuffer(BufferPoolManager &bp_manager, int max_pages = 16);
  virtual ~DiskDoubleWriteBuffer();

  /**
   * 打开磁盘中的共享表空间文件
   */
  RC open_file(const char *filename);

  /**
   * 将buffer中的页全部写入磁盘，并且清空buffer
   * TODO 目前的解决方案是等buffer装满后再刷盘，可能会导致程序卡住一段时间
   */
  RC flush_page();

  /**
   * 将页面加入buffer，并且写入磁盘中的共享表空间
   */
  RC add_page(DiskBufferPool *bp, PageNum page_num, Page &page) override;

  RC read_page(DiskBufferPool *bp, PageNum page_num, Page &page) override;

  /**
   * @brief 清空所有与指定buffer pool关联的页面
   */
  RC clear_pages(DiskBufferPool *bp) override;

  /**
   * 将共享表空间的页读入buffer
   */
  RC recover();

private:
  /**
   * 将buffer中的页面写入对应的磁盘
   */
  RC write_page(DoubleWritePage *page);

  /**
   * 将页面写到当前double write buffer文件中
   * @details 每次页面更新都应该写入到磁盘中。保证double write buffer
   * 内存和文件中的数据都是最新的。
   */
  RC write_page_internal(DoubleWritePage *page);

  /**
   * @brief 将磁盘文件中的内容加载到内存中。在启动时调用
   */
  RC load_pages();

private:
  int                     file_desc_ = -1;
  int                     max_pages_ = 0;
  common::Mutex           lock_;
  BufferPoolManager      &bp_manager_;
  DoubleWriteBufferHeader header_;

  unordered_map<DoubleWritePageKey, DoubleWritePage *, DoubleWritePageKeyHash> dblwr_pages_;
};

class VacuousDoubleWriteBuffer : public DoubleWriteBuffer
{
public:
  virtual ~VacuousDoubleWriteBuffer() = default;

  /**
   * 将页面加入buffer，并且写入磁盘中的共享表空间
   */
  RC add_page(DiskBufferPool *bp, PageNum page_num, Page &page) override;

  RC read_page(DiskBufferPool *bp, PageNum page_num, Page &page) override { return RC::BUFFERPOOL_INVALID_PAGE_NUM; }

  /**
   * @brief 清空所有与指定buffer pool关联的页面
   */
  RC clear_pages(DiskBufferPool *bp) override { return RC::SUCCESS; }
};
