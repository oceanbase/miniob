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
// Created by Longda on 2021/5/11.
//
#ifndef __OBSERVER_STORAGE_DEFAULT_ENGINE_H__
#define __OBSERVER_STORAGE_DEFAULT_ENGINE_H__

#include <string>
#include <map>

#include "storage/common/db.h"

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
  RC create_table(const char *dbname, const char *relation_name, int attribute_count, const AttrInfo *attributes);

  /**
   * 销毁名为relName的表以及在该表上建立的所有索引
   * @param relName
   * @return
   */
  RC drop_table(const char *dbname, const char *relation_name);

  /**
   * 该函数在关系relName的属性attrName上创建名为indexName的索引。
   * 函数首先检查在标记属性上是否已经存在一个索引，
   * 如果存在，则返回一个非零的错误码。
   * 否则，创建该索引。
   * 创建索引的工作包括：①创建并打开索引文件；
   * ②逐个扫描被索引的记录，并向索引文件中插入索引项；③关闭索引
   * @param indexName
   * @param relName
   * @param attrName
   * @return
   */
  RC create_index(Trx *trx, const char *dbname, const char *relation_name, const char *index_name, const char *attribute_name);

  /**
   * 该函数用来删除名为indexName的索引。
   * 函数首先检查索引是否存在，如果不存在，则返回一个非零的错误码。否则，销毁该索引
   * @param index_name 
   * @return
   */
  RC drop_index(Trx *trx, const char *dbname, const char *relation_name, const char *index_name);

  /**
   * 该函数用来在relName表中插入具有指定属性值的新元组，
   * nValues为属性值个数，values为对应的属性值数组。
   * 函数根据给定的属性值构建元组，调用记录管理模块的函数插入该元组，
   * 然后在该表的每个索引中为该元组创建合适的索引项
   * @param relName
   * @param nValues
   * @param values
   * @return
   */
  RC insert_record(Trx * trx, const char *dbname, const char *relation_name, int value_num, const Value *values);

  /**
   * 该函数用来删除relName表中所有满足指定条件的元组以及该元组对应的索引项。
   * 如果没有指定条件，则此方法删除relName关系中所有元组。
   * 如果包含多个条件，则这些条件之间为与关系
   * @param relName
   * @param nConditions
   * @param conditions
   * @return
   */
  RC delete_record(Trx *trx, const char *dbname, const char *relation_name,
                           int condition_num, const Condition *conditions, int *deleted_count);

  /**
   * 该函数用于更新relName表中所有满足指定条件的元组，
   * 在每一个更新的元组中将属性attrName的值设置为一个新的值。
   * 如果没有指定条件，则此方法更新relName中所有元组。
   * 如果要更新一个被索引的属性，应当先删除每个被更新元组对应的索引条目，然后插入一个新的索引条目
   * @param relName
   * @param attrName
   * @param value
   * @param nConditions
   * @param conditions
   * @return
   */
  RC update_record(Trx * trx, const char *dbname, const char *relation_name, const char *attribute_name, const Value *value,
                            int condition_num, const Condition *conditions, int *updated_count);

public:
  Db *find_db(const char *dbname) const;
  Table *find_table(const char * dbname, const char *table_name) const;

  RC sync();

public:
  static DefaultHandler &get_default();
private:
  std::string base_dir_;
  std::string db_dir_;
  std::map<std::string, Db*>          opened_dbs_;
}; // class Handler

#endif // __OBSERVER_STORAGE_DEFAULT_ENGINE_H__