
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

#include "common/lang/bitmap.h"
#include "storage/buffer/disk_buffer_pool.h"
#include "storage/record/record.h"
#include "storage/trx/latch_memo.h"
#include <limits>
#include <sstream>

class ConditionFilter;
class RecordPageHandler;
class Trx;
class Table;

/**
 * @brief 这里负责管理在一个文件上表记录(行)的组织/管理
 * @defgroup RecordManager
 *
 * @details 表记录管理的内容包括如何在文件上存放、读取、检索。也就是记录的增删改查。
 * 这里的文件都会被拆分成页面，每个页面都有一样的大小。更详细的信息可以参考BufferPool。
 * 按照BufferPool的设计，第一个页面用来存放BufferPool本身的元数据，比如当前文件有多少页面、已经分配了多少页面、
 * 每个页面的分配状态等。所以第一个页面对RecordManager来说没有作用。RecordManager 本身没有再单独拿一个页面
 * 来存放元数据，每一个页面都存放了一个页面头信息，也就是每个页面都有 RecordManager 的元数据信息，可以参考
 * PageHeader，这虽然有点浪费但是做起来简单。
 *
 * 对单个页面来说，最开始是一个页头，然后接着就是一行行记录（会对齐）。
 * 如何标识一个记录，或者定位一个记录？
 * 使用RID，即record identifier。使用 page num 表示所在的页面，slot num 表示当前在页面中的位置。因为这里的
 * 记录都是定长的，所以根据slot num 可以直接计算出记录的起始位置。
 * 问题1：那么如果记录不是定长的，还能使用 slot num 吗？
 * 问题2：如何更有效地存放不定长数据呢？
 * 问题3：如果一个页面不能存放一个记录，那么怎么组织记录存放效果更好呢？
 *
 * 按照上面的描述，这里提供了几个类，分别是：
 * - RecordFileHandler：管理整个文件/表的记录增删改查
 * - RecordPageHandler：管理单个页面上记录的增删改查
 * - RecordFileScanner：可以用来遍历整个文件上的所有记录
 * - RecordPageIterator：可以用来遍历指定页面上的所有记录
 * - PageHeader：每个页面上都会记录的页面头信息
 */

/**
 * @brief 数据文件，按照页面来组织，每一页都存放一些记录/数据行
 * @ingroup RecordManager
 * @details 每一页都有一个这样的页头，虽然看起来浪费，但是现在就简单的这么做
 * 从这个页头描述的信息来看，当前仅支持定长行/记录。如果要支持变长记录，
 * 或者超长（超出一页）的记录，这么做是不合适的。
 */
struct PageHeader
{
  int32_t record_num;           ///< 当前页面记录的个数
  int32_t record_real_size;     ///< 每条记录的实际大小
  int32_t record_size;          ///< 每条记录占用实际空间大小(可能对齐)
  int32_t record_capacity;      ///< 最大记录个数
  int32_t first_record_offset;  ///< 第一条记录的偏移量
};

/**
 * @brief 遍历一个页面中每条记录的iterator
 * @ingroup RecordManager
 */
class RecordPageIterator
{
public:
  RecordPageIterator();
  ~RecordPageIterator();

  /**
   * @brief 初始化一个迭代器
   *
   * @param record_page_handler 负责某个页面上记录增删改查的对象
   * @param start_slot_num      从哪个记录开始扫描，默认是0
   */
  void init(RecordPageHandler &record_page_handler, SlotNum start_slot_num = 0);

  /**
   * @brief 判断是否有下一个记录
   */
  bool has_next();

  /**
   * @brief 读取下一个记录到record中包括RID和数据，并更新下一个记录位置next_slot_num_
   *
   * @param record 返回的下一个记录
   */
  RC next(Record &record);

  /**
   * 该迭代器是否有效
   */
  bool is_valid() const { return record_page_handler_ != nullptr; }

private:
  RecordPageHandler *record_page_handler_ = nullptr;
  PageNum            page_num_            = BP_INVALID_PAGE_NUM;
  common::Bitmap     bitmap_;             ///< bitmap 的相关信息可以参考 RecordPageHandler 的说明
  SlotNum            next_slot_num_ = 0;  ///< 当前遍历到了哪一个slot
};

/**
 * @brief 负责处理一个页面中各种操作，比如插入记录、删除记录或者查找记录
 * @ingroup RecordManager
 * @details 当前定长记录模式下每个页面的组织大概是这样的：
 * @code
 * | PageHeader | record allocate bitmap |
 * |------------|------------------------|
 * | record1 | record2 | ..... | recordN |
 * @endcode
 */
class RecordPageHandler
{
public:
  RecordPageHandler() = default;
  ~RecordPageHandler();

  /**
   * @brief 初始化
   *
   * @param buffer_pool 关联某个文件时，都通过buffer pool来做读写文件
   * @param page_num    当前处理哪个页面
   * @param readonly    是否只读。在访问页面时，需要对页面加锁
   */
  RC init(DiskBufferPool &buffer_pool, PageNum page_num, bool readonly);

  /**
   * @brief 数据库恢复时，与普通的运行场景有所不同，不做任何并发操作，也不需要加锁
   *
   * @param buffer_pool 关联某个文件时，都通过buffer pool来做读写文件
   * @param page_num    操作的页面编号
   */
  RC recover_init(DiskBufferPool &buffer_pool, PageNum page_num);

  /**
   * @brief 对一个新的页面做初始化，初始化关于该页面记录信息的页头PageHeader
   *
   * @param buffer_pool 关联某个文件时，都通过buffer pool来做读写文件
   * @param page_num    当前处理哪个页面
   * @param record_size 每个记录的大小
   */
  RC init_empty_page(DiskBufferPool &buffer_pool, PageNum page_num, int record_size);

  /**
   * @brief 操作结束后做的清理工作，比如释放页面、解锁
   */
  RC cleanup();

  /**
   * @brief 插入一条记录
   *
   * @param data 要插入的记录
   * @param rid  如果插入成功，通过这个参数返回插入的位置
   */
  RC insert_record(const char *data, RID *rid);

  /**
   * @brief 数据库恢复时，在指定位置插入数据
   *
   * @param data 要插入的数据行
   * @param rid  插入的位置
   */
  RC recover_insert_record(const char *data, const RID &rid);

  /**
   * @brief 删除指定的记录
   *
   * @param rid 要删除的记录标识
   */
  RC delete_record(const RID *rid);

  /**
   * @brief 获取指定位置的记录数据
   *
   * @param rid 指定的位置
   * @param rec 返回指定的数据。这里不会将数据复制出来，而是使用指针，所以调用者必须保证数据使用期间受到保护
   */
  RC get_record(const RID *rid, Record *rec);

  /**
   * @brief 返回该记录页的页号
   */
  PageNum get_page_num() const;

  /**
   * @brief 当前页面是否已经没有空闲位置插入新的记录
   */
  bool is_full() const;

protected:
  /**
   * @details
   * 前面在计算record_capacity时并没有考虑对齐，但第一个record需要8字节对齐
   * 因此按此前计算的record_capacity，最后一个记录的部分数据可能会被挤出页面
   * 所以需要对record_capacity进行修正，保证记录不会溢出
   */
  void fix_record_capacity()
  {
    int32_t last_record_offset =
        page_header_->first_record_offset + page_header_->record_capacity * page_header_->record_size;
    while (last_record_offset > BP_PAGE_DATA_SIZE) {
      page_header_->record_capacity -= 1;
      last_record_offset -= page_header_->record_size;
    }
  }

  /**
   * @brief 获取指定槽位的记录数据
   *
   * @param 指定的记录槽位
   */
  char *get_record_data(SlotNum slot_num)
  {
    return frame_->data() + page_header_->first_record_offset + (page_header_->record_size * slot_num);
  }

protected:
  DiskBufferPool *disk_buffer_pool_ = nullptr;  ///< 当前操作的buffer pool(文件)
  Frame *frame_ = nullptr;  ///< 当前操作页面关联的frame(frame的更多概念可以参考buffer pool和frame)
  bool   readonly_         = false;    ///< 当前的操作是否都是只读的
  PageHeader *page_header_ = nullptr;  ///< 当前页面上页面头
  char       *bitmap_      = nullptr;  ///< 当前页面上record分配状态信息bitmap内存起始位置

private:
  friend class RecordPageIterator;
};

/**
 * @brief 管理整个文件中记录的增删改查
 * @ingroup RecordManager
 * @details 整个文件的组织格式请参考该文件中最前面的注释
 */
class RecordFileHandler
{
public:
  RecordFileHandler() = default;
  ~RecordFileHandler();

  /**
   * @brief 初始化
   *
   * @param buffer_pool 当前操作的是哪个文件
   */
  RC init(DiskBufferPool *buffer_pool);

  /**
   * @brief 关闭，做一些资源清理的工作
   */
  void close();

  /**
   * @brief 从指定文件中删除指定槽位的记录
   *
   * @param rid 待删除记录的标识符
   */
  RC delete_record(const RID *rid);

  /**
   * @brief 插入一个新的记录到指定文件中，并返回该记录的标识符
   *
   * @param data        纪录内容
   * @param record_size 记录大小
   * @param rid         返回该记录的标识符
   */
  RC insert_record(const char *data, int record_size, RID *rid);

  /**
   * @brief 数据库恢复时，在指定文件指定位置插入数据
   *
   * @param data        记录内容
   * @param record_size 记录大小
   * @param rid         要插入记录的指定标识符
   */
  RC recover_insert_record(const char *data, int record_size, const RID &rid);

  /**
   * @brief 获取指定文件中标识符为rid的记录内容到rec指向的记录结构中
   * @param page_handler[in]
   * 访问记录时，会拿住一些资源不释放，比如页面锁，使用这个对象保存相关的资源，并在析构时会自动释放
   * @param rid 想要获取的记录ID
   * @param readonly 获取的记录是只读的还是需要修改的
   * @param rec[out] 通过这个参数返回获取到的记录
   * @note rec 参数返回的记录并不会复制数据内存。page_handler 对象会拿着相关的资源，比如 pin 住页面和加上页面锁。
   *       如果page_handler 释放了，那也不能再访问rec对象了。
   */
  RC get_record(RecordPageHandler &page_handler, const RID *rid, bool readonly, Record *rec);

  /**
   * @brief 与get_record类似，访问某个记录，并提供回调函数来操作相应的记录
   *
   * @param rid 想要访问的记录ID
   * @param readonly 是否会修改记录
   * @param visitor  访问记录的回调函数
   */
  RC visit_record(const RID &rid, bool readonly, std::function<void(Record &)> visitor);

private:
  /**
   * @brief 初始化当前没有填满记录的页面，初始化free_pages_成员
   */
  RC init_free_pages();

private:
  DiskBufferPool             *disk_buffer_pool_ = nullptr;
  std::unordered_set<PageNum> free_pages_;  ///< 没有填充满的页面集合
  common::Mutex               lock_;  ///< 当编译时增加-DCONCURRENCY=ON 选项时，才会真正的支持并发
};

/**
 * @brief 遍历某个文件中所有记录
 * @ingroup RecordManager
 * @details 遍历所有的页面，同时访问这些页面中所有的记录
 */
class RecordFileScanner
{
public:
  RecordFileScanner() = default;
  ~RecordFileScanner();

  /**
   * @brief 打开一个文件扫描。
   * @details 如果条件不为空，则要对每条记录进行条件比较，只有满足所有条件的记录才被返回
   * @param table            遍历的哪张表
   * @param buffer_pool      访问的文件
   * @param readonly         当前是否只读操作。访问数据时，需要对页面加锁。比如
   *                         删除时也需要遍历找到数据，然后删除，这时就需要加写锁
   * @param condition_filter 做一些初步过滤操作
   */
  RC open_scan(Table *table, DiskBufferPool &buffer_pool, Trx *trx, bool readonly, ConditionFilter *condition_filter);

  /**
   * @brief 关闭一个文件扫描，释放相应的资源
   */
  RC close_scan();

  /**
   * @brief 判断是否还有数据
   * @details 判断完成后调用next获取下一条数据
   */
  bool has_next();

  /**
   * @brief 获取下一条记录
   *
   * @param record 返回的下一条记录
   *
   * @details 获取下一条记录之前先调用has_next()判断是否还有数据
   */
  RC next(Record &record);

private:
  /**
   * @brief 获取该文件中的下一条记录
   */
  RC fetch_next_record();

  /**
   * @brief 获取一个页面内的下一条记录
   */
  RC fetch_next_record_in_page();

private:
  // TODO 对于一个纯粹的record遍历器来说，不应该关心表和事务
  Table *table_ = nullptr;  ///< 当前遍历的是哪张表。这个字段仅供事务函数使用，如果设计合适，可以去掉

  DiskBufferPool *disk_buffer_pool_ = nullptr;  ///< 当前访问的文件
  Trx            *trx_              = nullptr;  ///< 当前是哪个事务在遍历
  bool            readonly_         = false;    ///< 遍历出来的数据，是否可能对它做修改

  BufferPoolIterator bp_iterator_;                 ///< 遍历buffer pool的所有页面
  ConditionFilter   *condition_filter_ = nullptr;  ///< 过滤record
  RecordPageHandler  record_page_handler_;         ///< 处理文件某页面的记录
  RecordPageIterator record_page_iterator_;        ///< 遍历某个页面上的所有record
  Record             next_record_;                 ///< 获取的记录放在这里缓存起来
};