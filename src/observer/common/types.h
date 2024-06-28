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
// Created by Wangyunlai on 2022/6/23.
//

#pragma once

#include <inttypes.h>
#include <stdint.h>

/// 磁盘文件，包括存放数据的文件和索引(B+-Tree)文件，都按照页来组织
/// 每一页都有一个编号，称为PageNum
using PageNum = int32_t;

/// 数据文件中按照页来组织，每一页会存放一些行数据(row)，或称为记录(record)
/// 每一行(row/record)，都占用一个槽位(slot)，这些槽有一个编号，称为SlotNum
using SlotNum = int32_t;

/// LSN for log sequence number
using LSN = int64_t;

#define LSN_FORMAT PRId64

/**
 * @brief 读写模式
 * @details 原来的代码中有大量的true/false来表示是否只读，这种代码不易于阅读
 */
enum class ReadWriteMode
{
  READ_ONLY,
  READ_WRITE
};

/**
 * @brief 存储格式
 * @details 当前仅支持行存格式（ROW_FORMAT）以及 PAX 存储格式(PAX_FORMAT)。
 */
enum class StorageFormat
{
  UNKNOWN_FORMAT = 0,
  ROW_FORMAT,
  PAX_FORMAT
};

/**
 * @brief 执行引擎模式
 * @details 当前支持按行处理（TUPLE_ITERATOR）以及按批处理(CHUNK_ITERATOR)两种模式。
 */
enum class ExecutionMode
{
  UNKNOWN_MODE = 0,
  TUPLE_ITERATOR,
  CHUNK_ITERATOR
};

/// page的CRC校验和
using CheckSum = unsigned int;
