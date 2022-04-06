/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

// Created by Meiyi & wangyunlai.wyl on 2021/5/18.
//

#ifndef __OBSERVER_STORAGE_COMMON_META_UTIL_H_
#define __OBSERVER_STORAGE_COMMON_META_UTIL_H_

#include <string>

static const char *TABLE_META_SUFFIX = ".table";
static const char *TABLE_META_FILE_PATTERN = ".*\\.table$";
static const char *TABLE_DATA_SUFFIX = ".data";
static const char *TABLE_INDEX_SUFFIX = ".index";

std::string table_meta_file(const char *base_dir, const char *table_name);
std::string table_data_file(const char *base_dir, const char *table_name);
std::string table_index_file(const char *base_dir, const char *table_name, const char *index_name);

#endif  //__OBSERVER_STORAGE_COMMON_META_UTIL_H_
