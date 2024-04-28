/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

// Created by wangyunlai.wyl on 2021/5/18.
//

#include <filesystem>

#include "storage/common/meta_util.h"

using namespace std;

string db_meta_file(const char *base_dir, const char *db_name)
{
  filesystem::path db_dir = filesystem::path(base_dir);
  return db_dir / (string(db_name) + DB_META_SUFFIX);
}

string table_meta_file(const char *base_dir, const char *table_name)
{
  return filesystem::path(base_dir) / (string(table_name) + TABLE_META_SUFFIX);
}

string table_data_file(const char *base_dir, const char *table_name)
{
  return filesystem::path(base_dir) / (string(table_name) + TABLE_DATA_SUFFIX);
}

string table_index_file(const char *base_dir, const char *table_name, const char *index_name)
{
  return filesystem::path(base_dir) / (string(table_name) + "-" + index_name + TABLE_INDEX_SUFFIX);
}
