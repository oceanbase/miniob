/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "catalog/catalog.h"

const TableStats &Catalog::get_table_stats(int table_id)
{
  lock_guard<mutex> lock(mutex_);
  return table_stats_[table_id];
}

void Catalog::update_table_stats(int table_id, const TableStats &table_stats)
{
  lock_guard<mutex> lock(mutex_);
  table_stats_[table_id] = table_stats;
}