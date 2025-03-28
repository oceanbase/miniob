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

#include "storage/record/record_scanner.h"
#include "oblsm/include/ob_lsm_iterator.h"
#include "oblsm/include/ob_lsm.h"
#include "sql/expr/tuple.h"
#include "storage/trx/trx.h"

using namespace oceanbase;
class LsmRecordScanner : public RecordScanner
{
public:
  LsmRecordScanner(Table *table, ObLsm *oblsm, Trx *trx) : table_(table), oblsm_(oblsm), trx_(trx) {}
  ~LsmRecordScanner() = default;

  /**
   * @brief 打开 RecordScanner
   */
  RC open_scan() override;

  /**
   * @brief 关闭 RecordScanner，释放相应的资源
   */
  RC close_scan() override;

  /**
   * @brief 获取下一条记录
   *
   * @param record 返回的下一条记录
   */
  RC next(Record &record) override;

private:
  Table         *table_    = nullptr;
  ObLsm         *oblsm_    = nullptr;
  Trx           *trx_      = nullptr;
  ObLsmIterator *lsm_iter_ = nullptr;
  RowTuple       tuple_;
  Record         record_;
};