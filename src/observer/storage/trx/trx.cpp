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
// Created by Wangyunlai on 2021/5/24.
//

#include <atomic>

#include "common/lang/string.h"
#include "common/log/log.h"
#include "storage/field/field.h"
#include "storage/field/field_meta.h"
#include "storage/record/record_manager.h"
#include "storage/table/table.h"
#include "storage/trx/mvcc_trx.h"
#include "storage/trx/trx.h"
#include "storage/trx/vacuous_trx.h"

TrxKit *TrxKit::create(const char *name)
{
  TrxKit *trx_kit = nullptr;
  if (common::is_blank(name) || 0 == strcasecmp(name, "vacuous")) {
    trx_kit = new VacuousTrxKit();
  } else if (0 == strcasecmp(name, "mvcc")) {
    trx_kit = new MvccTrxKit();
  } else {
    LOG_ERROR("unknown trx kit name. name=%s", name);
    return nullptr;
  }

  RC rc = trx_kit->init();
  if (OB_FAIL(rc)) {
    LOG_ERROR("failed to init trx kit. name=%s, rc=%s", name, strrc(rc));
    delete trx_kit;
    trx_kit = nullptr;
  }
  
  return trx_kit;
}
