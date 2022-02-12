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
// Created by Meiyi & Wangyunlai on 2021/5/11.
//

#ifndef __OBSERVER_STORAGE_COMMON_INDEX_H_
#define __OBSERVER_STORAGE_COMMON_INDEX_H_

#include <stddef.h>
#include <vector>

#include "rc.h"
#include "storage/common/index_meta.h"
#include "storage/common/field_meta.h"
#include "storage/common/record_manager.h"

class IndexDataOperator {
public:
  virtual ~IndexDataOperator() = default;
  virtual int compare(const void *data1, const void *data2) const = 0;
  virtual size_t hash(const void *data) const = 0;
};

class IndexScanner;

class Index {

public:
  Index() = default;
  virtual ~Index() = default;

  const IndexMeta &index_meta() const
  {
    return index_meta_;
  }

  virtual RC insert_entry(const char *record, const RID *rid) = 0;
  virtual RC delete_entry(const char *record, const RID *rid) = 0;

  virtual IndexScanner *create_scanner(CompOp comp_op, const char *value) = 0;

  virtual RC sync() = 0;

protected:
  RC init(const IndexMeta &index_meta, const FieldMeta &field_meta);

protected:
  IndexMeta index_meta_;
  FieldMeta field_meta_;  /// 当前实现仅考虑一个字段的索引
};

class IndexScanner {
public:
  IndexScanner() = default;
  virtual ~IndexScanner() = default;

  virtual RC next_entry(RID *rid) = 0;
  virtual RC destroy() = 0;
};

#endif  // __OBSERVER_STORAGE_COMMON_INDEX_H_