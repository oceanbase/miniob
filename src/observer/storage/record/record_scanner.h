/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#pragma once

#include "storage/record/record.h"
#include "storage/common/condition_filter.h"

/**
 * @brief 遍历某个表中所有记录
 * @ingroup RecordManager
 */
class RecordScanner
{
public:
  RecordScanner()          = default;
  virtual ~RecordScanner() = default;

  /**
   * @brief 打开 RecordScanner
   */
  virtual RC open_scan() = 0;

  /**
   * @brief 关闭 RecordScanner，释放相应的资源
   */
  virtual RC close_scan() = 0;

  /**
   * @brief 获取下一条记录
   *
   * @param record 返回的下一条记录
   */
  virtual RC next(Record &record) = 0;
};