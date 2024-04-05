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
// Created by Wangyunlai on 2023/03/07.
//

#pragma once

#include "common/types.h"
#include <stdint.h>
#include <string>

using TrxID = int32_t;

static constexpr int BP_INVALID_PAGE_NUM = -1;

static constexpr PageNum BP_HEADER_PAGE = 0;

static constexpr const int BP_PAGE_SIZE      = (1 << 13);
static constexpr const int BP_PAGE_DATA_SIZE = (BP_PAGE_SIZE - sizeof(PageNum) - sizeof(LSN) - sizeof(CheckSum));

static constexpr const int FILE_NAME_SIZE = (1 << 10);
static constexpr const int DW_PAGE_SIZE   = FILE_NAME_SIZE + BP_PAGE_SIZE + sizeof(PageNum);

/**
 * @brief 表示一个页面，可能放在内存或磁盘上
 * @ingroup BufferPool
 */
struct Page
{
  PageNum  page_num;
  LSN      lsn;
  CheckSum check_sum;
  char     data[BP_PAGE_DATA_SIZE];
};

class DoubleWritePage
{
public:
  DoubleWritePage(PageNum page_num, std::string file_name, Page page) : page_(page)
  {
    for (size_t i = 0; i < file_name.size(); i++) {
      file_name_[i] = file_name[i];
    }
    file_name_[file_name.size()] = '\0';
  }

  std::string get_file_name() { return file_name_; }

  Page &get_page() { return page_; }

private:
  char file_name_[FILE_NAME_SIZE];
  Page page_;
};
