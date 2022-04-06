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
// Created by Meiyi
// Rewritten by Longda on 2021/4/14.
//

#ifndef __OBSERVER_HANDLER_HANDLER_H__
#define __OBSERVER_HANDLER_HANDLER_H__

#define MAX_NUM 20
#define MAX_REL_NAME 20
#define MAX_ATTR_NAME 20
#define MAX_ERROR_MESSAGE 20
#define MAX_DATA 50

#include <stdlib.h>
#include <string.h>

#include "rc.h"

//属性结构体
typedef struct {
  char *relName;   // relation name (may be NULL) 表名
  char *attrName;  // attribute name              属性名
} RelAttr;

typedef enum {
  EQual,   //"="			0
  LEqual,  //"<="           1
  NEqual,  //"<>"			2
  LessT,   //"<"			3
  GEqual,  //">="			4
  GreatT,  //">"            5
  NO_OP
} CompOp;

//属性值类型
typedef enum { chars, ints, floats } AttrType;
//属性值
typedef struct _Value Value;
struct _Value {
  AttrType type;  // type of value
  void *data;     // value
};

typedef struct _Condition {
  int bLhsIsAttr;   // TRUE if left-hand side is an attribute
                    // 1时，操作符右边是属性，0时，是属性值
  Value lhsValue;   // left-hand side value if bLhsIsAttr = FALSE
  RelAttr lhsAttr;  // left-hand side attribute
  CompOp op;        // comparison operator
  int bRhsIsAttr;   // TRUE if right-hand side is an attribute
                    // 1时，操作符右边是属性，0时，是属性值
  //   and not a value
  RelAttr rhsAttr;  // right-hand side attribute if bRhsIsAttr = TRUE 右边的属性
  Value rhsValue;   // right-hand side value if bRhsIsAttr = FALSE
} Condition;

// struct of select
typedef struct {
  int nSelAttrs;                  // Length of attrs in Select clause
  RelAttr *selAttrs[MAX_NUM];     // attrs in Select clause
  int nRelations;                 // Length of relations in Fro clause
  char *relations[MAX_NUM];       // relations in From clause
  int nConditions;                // Length of conditions in Where clause
  Condition conditions[MAX_NUM];  // conditions in Where clause
} Selects;

// struct of insert
typedef struct {
  char *relName;          // Relation to insert into
  int nValues;            // Length of values
  Value values[MAX_NUM];  // values to insert
} Inserts;

// struct of delete
typedef struct {
  char *relName;                  // Relation to delete from
  int nConditions;                // Length of conditions in Where clause
  Condition conditions[MAX_NUM];  // conditions in Where clause
} Deletes;

// struct of update
typedef struct {
  char *relName;                  // Relation to update
  char *attrName;                 // Attribute to update
  Value value;                    // update value
  int nConditions;                // Length of conditions in Where clause
  Condition conditions[MAX_NUM];  // conditions in Where clause
} Updates;

// struct of AttrInfo
typedef struct _AttrInfo AttrInfo;
struct _AttrInfo {
  char *attrName;     // Attribute name
  AttrType attrType;  // Type of attribute
  int attrLength;     // Length of attribute
};
// struct of craete_table
typedef struct {
  char *relName;                 // Relation name
  int attrCount;                 // Length of attribute
  AttrInfo attributes[MAX_NUM];  // attributes
} CreateTable;

// struct of drop_table
typedef struct {
  char *relName;  // Relation name
} DropTable;

// struct of create_index
typedef struct {
  char *indexName;  // Index name
  char *relName;    // Relation name
  char *attrName;   // Attribute name
} CreateIndex;

// struct of  drop_index
typedef struct {
  char *indexName;  // Index name

} DropIndex;

// union of sql_structs
union sqls {
  Selects sel;
  Inserts ins;
  Deletes del;
  Updates upd;
  CreateTable cret;
  DropTable drt;
  CreateIndex crei;
  DropIndex dri;
  char *errors;
};

// struct of flag and sql_struct
typedef struct {
  int flag; /*match to the sqls
               0--error;1--select;2--insert;3--update;4--delete;5--create
               table;6--drop table;7--create index;8--drop
               index;9--help;10--exit;*/
  union sqls sstr;
} sqlstr;

/**
 * 在路径dbPath下创建一个名为dbName的空库，生成相应的系统文件。
 * 接口要求：一个数据库对应一个文件夹， dbName即为文件夹名，
 * 文件夹中创建两个系统文件SYSTABLES和SYSCOLUMNS，文件名不能带后缀。
 * dbPath为创建数据库的路径名，该路径名的最后不要含“\”
 * @param dbpath
 * @param dbname
 * @return
 */
RC createDB(char *dbpath, char *dbname);

/**
 * 删除一个数据库，dbName为要删除的数据库对应文件夹的路径名。
 * 接口要求：参数dbName等于CreateDB函数中两个参数的拼接，即dbPath+”\”+dbName
 * @param dbname
 * @return
 */
RC dropDB(char *dbname);

/**
 * 改变系统的当前数据库为dbName对应的文件夹中的数据库。接口要求同dropDB
 * @param dbname
 * @return
 */
RC openDB(char *dbname);

/**
 * 关闭当前数据库。
 * 该操作将关闭当前数据库中打开的所有文件，关闭文件操作将自动使所有相关的缓冲区页面更新到磁盘
 * @return
 */
RC closeDB();

/**
 * 执行一条除SELECT之外的SQL语句，如果执行成功，返回SUCCESS，否则返回错误码。
 * 注意：此函数是提供给测试程序专用的接口
 * @param sql
 * @return
 */
RC execute(char *sql);

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
RC createTable(char *relName, int attrCount, AttrInfo *attributes);

/**
 * 销毁名为relName的表以及在该表上建立的所有索引
 * @param relName
 * @return
 */
RC dropTable(char *relName);

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
RC createIndex(char *indexName, char *relName, char *attrName);

/**
 * 该函数用来删除名为indexName的索引。
 * 函数首先检查索引是否存在，如果不存在，则返回一个非零的错误码。否则，销毁该索引
 * @param indexName
 * @return
 */
RC dropIndex(char *indexName);

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
RC insertRecord(char *relName, int nValues, Value *values);

/**
 * 该函数用来删除relName表中所有满足指定条件的元组以及该元组对应的索引项。
 * 如果没有指定条件，则此方法删除relName关系中所有元组。
 * 如果包含多个条件，则这些条件之间为与关系
 * @param relName
 * @param nConditions
 * @param conditions
 * @return
 */
RC deleteRecord(char *relName, int nConditions, Condition *conditions);

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
RC updateRecord(char *relName, char *attrName, Value *value, int nConditions, Condition *conditions);

#endif  //__OBSERVER_HANDLER_HANDLER_H__
