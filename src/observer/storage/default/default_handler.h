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
// Created by Meiyi & Longda on 2021/5/11.
//
#pragma once

#include "storage/db/db.h"
#include "common/lang/span.h"
#include "common/lang/map.h"
#include "common/lang/string.h"
#include "common/lang/memory.h"

class Trx;
class TrxKit;

/**
 * @brief 数据库存储引擎的入口
 * @details 参考MySQL，可以抽象handler层，作为SQL层与存储层交互的接口。但是当前还不具备这个条件。
 */
class DefaultHandler
{
public:
  DefaultHandler();

  virtual ~DefaultHandler() noexcept;

  /**
   * @brief 初始化存储引擎
   * @param base_dir 存储引擎的根目录。所有的数据库相关数据文件都放在这个目录下
   * @param trx_kit_name 使用哪种类型的事务模型
   * @param log_handler_name 使用哪种类型的日志处理器
   */
  RC   init(const char *base_dir, const char *trx_kit_name, const char *log_handler_name);
  void destroy();

  /**
   * @brief 创建一个数据库
   * @details 在路径base_dir下创建一个名为dbname的空库，生成相应的系统文件。
   * @param dbname 数据库名称
   */
  RC create_db(const char *dbname);

  /**
   * @brief 删除数据库
   * @details 当前并没有实现
   * @param dbname 数据库名称
   */
  RC drop_db(const char *dbname);

  /**
   * @brief 打开一个数据库
   * @param dbname 数据库名称
   */
  RC open_db(const char *dbname);

  /**
   * @brief 关闭指定数据库。
   * @details 该操作将关闭当前数据库中打开的所有文件，关闭文件操作将自动使所有相关的缓冲区页面更新到磁盘
   */
  RC close_db(const char *dbname);

  /**
   * @brief 在指定的数据库下创建一个表
   * @param dbname 数据库名称
   * @param relation_name 表名
   * @param attributes 属性信息
   */
  RC create_table(const char *dbname, const char *relation_name, span<const AttrInfoSqlNode> attributes);

  /**
   * @brief 删除指定数据库下的表
   * @details 当前没有实现。需要删除表在内存中和磁盘中的所有资源，包括表的索引文件。
   * @param dbname 数据库名称
   * @param relation_name 表名
   */
  RC drop_table(const char *dbname, const char *relation_name);

public:
  Db    *find_db(const char *dbname) const;
  Table *find_table(const char *dbname, const char *table_name) const;

  RC sync();

private:
  filesystem::path  base_dir_;          ///< 存储引擎的根目录
  filesystem::path  db_dir_;            ///< 数据库文件的根目录
  string            trx_kit_name_;      ///< 事务模型的名称
  string            log_handler_name_;  ///< 日志处理器的名称
  map<string, Db *> opened_dbs_;        ///< 打开的数据库
};
