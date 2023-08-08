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

#include <string>
#include <map>

#include "storage/db/db.h"

class Trx;

class DefaultHandler {
public:
  DefaultHandler();

  virtual ~DefaultHandler() noexcept;

  RC init(const char *base_dir);
  void destroy();

  /**
   * 在路径dbPath下创建一个名为dbName的空库，生成相应的系统文件。
   * 接口要求：一个数据库对应一个文件夹， dbName即为文件夹名，
   * 文件夹中创建两个系统文件SYSTABLES和SYSCOLUMNS，文件名不能带后缀。
   * dbPath为创建数据库的路径名，该路径名的最后不要含“\”
   * @param dbname
   * @return
   */
  RC create_db(const char *dbname);

  /**
   * 删除一个数据库，dbName为要删除的数据库对应文件夹的路径名。
   * 接口要求：参数dbName等于CreateDB函数中两个参数的拼接，即dbPath+”\”+dbName
   * @param dbname
   * @return
   */
  RC drop_db(const char *dbname);

  /**
   * 改变系统的当前数据库为dbName对应的文件夹中的数据库。接口要求同dropDB
   * @param dbname
   * @return
   */
  RC open_db(const char *dbname);

  /**
   * 关闭当前数据库。
   * 该操作将关闭当前数据库中打开的所有文件，关闭文件操作将自动使所有相关的缓冲区页面更新到磁盘
   * @return
   */
  RC close_db(const char *dbname);

  /**
   * 执行一条除SELECT之外的SQL语句，如果执行成功，返回SUCCESS，否则返回错误码。
   * 注意：此函数是提供给测试程序专用的接口
   * @param sql
   * @return
   */
  RC execute(const char *sql);

  /**
   * 创建一个名为relName的表。
   * 参数attrCount表示关系中属性的数量（取值为1到MAXATTRS之间）。
   * 参数attributes是一个长度为attrCount的数组。
   * 对于新关系中第i个属性，attributes数组中的第i个元素包含名称、
   * 类型和属性的长度（见AttrInfo结构定义）
   * @param relName
   * @param attrCount
   * @param attributes
   * @return
   */
  RC create_table(const char *dbname, const char *relation_name, int attribute_count, const AttrInfoSqlNode *attributes);

  /**
   * 销毁名为relName的表以及在该表上建立的所有索引
   * @param relName
   * @return
   */
  RC drop_table(const char *dbname, const char *relation_name);

public:
  Db *find_db(const char *dbname) const;
  Table *find_table(const char *dbname, const char *table_name) const;

  RC sync();

public:
  static void set_default(DefaultHandler *handler);
  static DefaultHandler &get_default();

private:
  std::string base_dir_;
  std::string db_dir_;
  std::map<std::string, Db *> opened_dbs_;
};  // class Handler
