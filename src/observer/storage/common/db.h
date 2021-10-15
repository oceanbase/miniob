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
// Created by Wangyunlai on 2021/5/12.
//

#ifndef __OBSERVER_STORAGE_COMMON_DB_H__
#define __OBSERVER_STORAGE_COMMON_DB_H__

#include <vector>
#include <string>
#include <unordered_map>

#include "rc.h"
#include "sql/parser/parse_defs.h"

class Table;

class Db {
public:
  Db() = default;
  ~Db();

  RC init(const char *name, const char *dbpath);

  RC create_table(const char *table_name, int attribute_count, const AttrInfo *attributes);

  Table *find_table(const char *table_name) const;

  const char *name() const;

  void all_tables(std::vector<std::string> &table_names) const;

  RC sync();
private:
  RC open_all_tables();

private:
  std::string   name_;
  std::string   path_;
  std::unordered_map<std::string, Table *>  opened_tables_;
};

#endif // __OBSERVER_STORAGE_COMMON_DB_H__