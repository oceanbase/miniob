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
// Created by Meiyi & Wangyunlai on 2021/5/11.
//

#pragma once

#include <stddef.h>
#include <vector>

#include "common/rc.h"
#include "storage/field/field_meta.h"
#include "storage/index/index_meta.h"
#include "storage/record/record_manager.h"

class IndexScanner;

/**
 * @brief 索引
 * @defgroup Index
 * @details 索引可能会有很多种实现，比如B+树、哈希表等，这里定义了一个基类，用于描述索引的基本操作。
 */

/**
 * @brief 索引基类
 * @ingroup Index
 */
class Index
{
public:
  Index()          = default;
  virtual ~Index() = default;

  const IndexMeta &index_meta() const { return index_meta_; }

  /**
   * @brief 插入一条数据
   *
   * @param record 插入的记录，当前假设记录是定长的
   * @param[out] rid    插入的记录的位置
   */
  virtual RC insert_entry(const char *record, const RID *rid) = 0;

  /**
   * @brief 删除一条数据
   *
   * @param record 删除的记录，当前假设记录是定长的
   * @param[in] rid   删除的记录的位置
   */
  virtual RC delete_entry(const char *record, const RID *rid) = 0;

  /**
   * @brief 创建一个索引数据的扫描器
   *
   * @param left_key 要扫描的左边界
   * @param left_len 左边界的长度
   * @param left_inclusive 是否包含左边界
   * @param right_key 要扫描的右边界
   * @param right_len 右边界的长度
   * @param right_inclusive 是否包含右边界
   */
  virtual IndexScanner *create_scanner(const char *left_key, int left_len, bool left_inclusive, const char *right_key,
      int right_len, bool right_inclusive) = 0;

  /**
   * @brief 同步索引数据到磁盘
   *
   */
  virtual RC sync() = 0;

protected:
  RC init(const IndexMeta &index_meta, const FieldMeta &field_meta);

protected:
  IndexMeta index_meta_;  ///< 索引的元数据
  FieldMeta field_meta_;  ///< 当前实现仅考虑一个字段的索引
};

/**
 * @brief 索引扫描器
 * @ingroup Index
 */
class IndexScanner
{
public:
  IndexScanner()          = default;
  virtual ~IndexScanner() = default;

  /**
   * 遍历元素数据
   * 如果没有更多的元素，返回RECORD_EOF
   */
  virtual RC next_entry(RID *rid) = 0;
  virtual RC destroy()            = 0;
};
