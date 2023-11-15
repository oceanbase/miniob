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
// Created by Wangyunlai on 2023/4/24.
//

#include "storage/trx/vacuous_trx.h"

using namespace std;

RC VacuousTrxKit::init() { return RC::SUCCESS; }

const vector<FieldMeta> *VacuousTrxKit::trx_fields() const { return nullptr; }

Trx *VacuousTrxKit::create_trx(CLogManager *) { return new VacuousTrx; }

Trx *VacuousTrxKit::create_trx(int32_t /*trx_id*/) { return nullptr; }

void VacuousTrxKit::destroy_trx(Trx *) {}

Trx *VacuousTrxKit::find_trx(int32_t /* trx_id */) { return nullptr; }

void VacuousTrxKit::all_trxes(std::vector<Trx *> &trxes) { return; }

////////////////////////////////////////////////////////////////////////////////

RC VacuousTrx::insert_record(Table *table, Record &record) { return table->insert_record(record); }

RC VacuousTrx::delete_record(Table *table, Record &record) { return table->delete_record(record); }

RC VacuousTrx::visit_record(Table *table, Record &record, bool readonly) { return RC::SUCCESS; }

RC VacuousTrx::start_if_need() { return RC::SUCCESS; }

RC VacuousTrx::commit() { return RC::SUCCESS; }

RC VacuousTrx::rollback() { return RC::SUCCESS; }
