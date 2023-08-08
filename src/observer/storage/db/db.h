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
// Created by Meiyi & Longda & Wangyunlai on 2021/5/12.
//

#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <memory>

#include "common/rc.h"
#include "sql/parser/parse_defs.h"

class Table;
class CLogManager;

/**
 * @brief 一个DB实例负责管理一批表
 * @details 当前DB的存储模式很简单，一个DB对应一个目录，所有的表和数据都放置在这个目录下。
 * 启动时，从指定的目录下加载所有的表和元数据。
 */
class Db
{
public:
  Db() = default;
  ~Db();

  /**
   * @brief 初始化一个数据库实例
   * @details 从指定的目录下加载指定名称的数据库。这里就会加载dbpath目录下的数据。
   * @param name   数据库名称
   * @param dbpath 当前数据库放在哪个目录下
   * @note 数据库不是放在dbpath/name下，是直接使用dbpath目录
   */
  RC init(const char *name, const char *dbpath);

  RC create_table(const char *table_name, int attribute_count, const AttrInfoSqlNode *attributes);

  Table *find_table(const char *table_name) const;
  Table *find_table(int32_t table_id) const;

  const char *name() const;

  void all_tables(std::vector<std::string> &table_names) const;

  RC sync();

  RC recover();

  CLogManager *clog_manager();

private:
  RC open_all_tables();

private:
  std::string name_;
  std::string path_;
  std::unordered_map<std::string, Table *> opened_tables_;
  std::unique_ptr<CLogManager> clog_manager_;

  /// 给每个table都分配一个ID，用来记录日志。这里假设所有的DDL都不会并发操作，所以相关的数据都不上锁
  int32_t next_table_id_ = 0;
};
