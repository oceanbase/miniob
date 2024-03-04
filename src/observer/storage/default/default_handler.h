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

#include <map>
#include <string>
#include <memory>

#include "storage/db/db.h"

class Trx;
class TrxKit;

class DefaultHandler
{
public:
  DefaultHandler();

  virtual ~DefaultHandler() noexcept;

  RC   init(const char *base_dir, const char *trx_kit_name);
  void destroy();

  /**
   * 在路径dbPath下创建一个名为dbName的空库，生成相应的系统文件。
   * 接口要求：一个数据库对应一个文件夹， dbName即为文件夹名，
   * dbPath为创建数据库的路径名，该路径名的最后不要含“\”
   * @param dbname 数据库名称
   */
  RC create_db(const char *dbname);

  /**
   * 删除一个数据库，dbName为要删除的数据库对应文件夹的路径名。
   * 接口要求：参数dbName等于CreateDB函数中两个参数的拼接，即dbPath+”\”+dbName
   * @param dbname 数据库名称
   */
  RC drop_db(const char *dbname);

  /**
   * 改变系统的当前数据库为dbName对应的文件夹中的数据库。接口要求同dropDB
   * @param dbname 数据库名称
   */
  RC open_db(const char *dbname);

  /**
   * 关闭当前数据库。
   * 该操作将关闭当前数据库中打开的所有文件，关闭文件操作将自动使所有相关的缓冲区页面更新到磁盘
   */
  RC close_db(const char *dbname);

  /**
   * 执行一条除SELECT之外的SQL语句，如果执行成功，返回SUCCESS，否则返回错误码。
   * 注意：此函数是提供给测试程序专用的接口
   * @param sql SQL语句
   */
  RC execute(const char *sql);

  /**
   * 创建一个名为relName的表。
   * 参数attrCount表示关系中属性的数量（取值为1到MAXATTRS之间）。
   * 参数attributes是一个长度为attrCount的数组。
   * 对于新关系中第i个属性，attributes数组中的第i个元素包含名称、
   * 类型和属性的长度（见AttrInfo结构定义）
   * @param dbname 数据库名称
   * @param relation_name 表名
   * @param attribute_count 属性数量
   * @param attributes 属性信息
   */
  RC create_table(
      const char *dbname, const char *relation_name, int attribute_count, const AttrInfoSqlNode *attributes);

  /**
   * 销毁名为relName的表以及在该表上建立的所有索引
   * @param dbname 数据库名称
   * @param relation_name 表名
   */
  RC drop_table(const char *dbname, const char *relation_name);

public:
  TrxKit &trx_kit() { return *trx_kit_; }
  BufferPoolManager &buffer_pool_manager() { return *buffer_pool_manager_; }
  LogHandler &log_handler() { return *log_handler_; }

public:
  Db    *find_db(const char *dbname) const;
  Table *find_table(const char *dbname, const char *table_name) const;

  RC sync();

private:
  std::filesystem::path                 base_dir_;
  std::filesystem::path                 db_dir_;
  std::string trx_kit_name_;
  std::map<std::string, Db *> opened_dbs_;
  std::unique_ptr<TrxKit>     trx_kit_;
  std::unique_ptr<BufferPoolManager> buffer_pool_manager_;
  std::unique_ptr<DiskLogHandler>    log_handler_;
};  // class Handler
