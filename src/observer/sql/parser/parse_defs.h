/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
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
//

#pragma once

#include <stddef.h>
#include <memory>
#include <vector>
#include <string>

#include "sql/parser/value.h"

/**
 * @brief 描述一个属性
 * @details 属性，或者说字段(column, field)
 * Rel -> Relation
 * Attr -> Attribute
 */
struct RelAttr 
{
  std::string relation_name;   ///< relation name (may be NULL) 表名
  std::string attribute_name;  ///< attribute name              属性名
};

/**
 * @brief 描述比较运算符
 */
enum CompOp 
{
  EQUAL_TO,     //"="     0
  LESS_EQUAL,   //"<="    1
  NOT_EQUAL,    //"<>"    2
  LESS_THAN,    //"<"     3
  GREAT_EQUAL,  //">="    4
  GREAT_THAN,   //">"     5
  NO_OP
};

/**
 * @brief 表示一个条件比较
 * @details 条件比较就是SQL查询中的 where a>b 这种。
 * 一个条件比较是有两部分组成的，称为左边和右边。
 * 左边和右边理论上都可以是任意的数据，比如是字段（属性，列），也可以是数值常量。
 * 这个结构中记录的仅仅支持字段和值。
 */
struct Condition
{
  int left_is_attr;    /// TRUE if left-hand side is an attribute
                       /// 1时，操作符左边是属性名，0时，是属性值
  Value left_value;    /// left-hand side value if left_is_attr = FALSE
  RelAttr left_attr;   /// left-hand side attribute
  CompOp comp;         /// comparison operator
  int right_is_attr;   /// TRUE if right-hand side is an attribute
                       /// 1时，操作符右边是属性名，0时，是属性值
  RelAttr right_attr;  /// right-hand side attribute if right_is_attr = TRUE 右边的属性
  Value right_value;   /// right-hand side value if right_is_attr = FALSE
};

/**
 * @brief 描述一个select语句
 * @details 一个正常的select语句描述起来比这个要复杂很多，这里做了简化。
 * 一个select语句由三部分组成，分别是select, from, where。
 * select部分表示要查询的字段，from部分表示要查询的表，where部分表示查询的条件。
 * 比如 from 中可以是多个表，也可以是另一个查询语句，这里仅仅支持表，也就是 relations。
 * where 条件 conditions，这里表示使用AND串联起来多个条件。正常的SQL语句会有OR，NOT等，
 * 甚至可以包含复杂的表达式。
 */
struct Selects
{
  std::vector<RelAttr> attributes;    ///< attributes in select clause
  std::vector<std::string> relations; ///< 查询的表
  std::vector<Condition> conditions;  ///< 查询条件，使用AND串联起来多个条件
};

/**
 * @brief 描述一个insert语句
 * @details 于Selects类似，也做了很多简化
 */
struct Inserts
{
  std::string relation_name;  ///< Relation to insert into
  std::vector<Value> values;  ///< 要插入的值
};

/**
 * @brief 描述一个delete语句
 */
struct Deletes
{
  std::string relation_name;  // Relation to delete from
  std::vector<Condition> conditions;
};

/**
 * @brief 描述一个update语句
 * 
 */
struct Updates
{
  std::string relation_name;   ///< Relation to update
  std::string attribute_name;  ///< 更新的字段，仅支持一个字段
  Value value;                 ///< 更新的值，仅支持一个字段
  std::vector<Condition> conditions;
};

/**
 * @brief 描述一个属性
 * @details 属性，或者说字段(column, field)
 * Rel -> Relation
 * Attr -> Attribute
 */
struct AttrInfo
{
  AttrType type;     ///< Type of attribute
  std::string name;  ///< Attribute name
  size_t length;     ///< Length of attribute
};

/**
 * @brief 描述一个create table语句
 * @details 这里也做了很多简化。
 */
struct CreateTable
{
  std::string relation_name;         // Relation name
  std::vector<AttrInfo> attr_infos;  // attributes
};

/**
 * @brief 描述一个drop table语句
 * 
 */
struct DropTable
{
  std::string relation_name;  /// 要删除的表名
};

/**
 * @brief 描述一个create index语句
 * @details 创建索引时，需要指定索引名，表名，字段名。
 * 正常的SQL语句中，一个索引可能包含了多个字段，这里仅支持一个字段。
 */
struct CreateIndex
{
  std::string index_name;      /// Index name
  std::string relation_name;   /// Relation name
  std::string attribute_name;  /// Attribute name
};

/**
 * @brief 描述一个drop index语句
 * 
 */
struct DropIndex
{
  std::string index_name;     ///< Index name
  std::string relation_name;  ///< Relation name
};

/**
 * @brief 描述一个desc table语句
 * @details desc table 是查询表结构信息的语句
 */
struct DescTable
{
  std::string relation_name;
};

/**
 * @brief 描述一个load data语句
 * @details 从文件导入数据到表中。文件中的每一行就是一条数据，每行的数据类型、字段个数都与表保持一致
 */
struct LoadData
{
  std::string relation_name;
  std::string file_name;
};

class Command;

/**
 * @brief 描述一个explain语句
 * @details 会创建operator的语句，才能用explain输出执行计划。
 * 一个command就是一个语句，比如select语句，insert语句等。
 * 可能改成SqlCommand更合适。
 */
struct Explain
{
  std::unique_ptr<Command> cmd;
};

/**
 * @brief 解析SQL语句出现了错误
 * @details 当前解析时并没有处理错误的行号和列号
 */
struct Error
{
  std::string error_msg;
  int line;
  int column;
};

/**
 * @brief 表示一个SQL语句的类型
 * 
 */
enum SqlCommandFlag
{
  SCF_ERROR = 0,
  SCF_SELECT,
  SCF_INSERT,
  SCF_UPDATE,
  SCF_DELETE,
  SCF_CREATE_TABLE,
  SCF_DROP_TABLE,
  SCF_CREATE_INDEX,
  SCF_DROP_INDEX,
  SCF_SYNC,
  SCF_SHOW_TABLES,
  SCF_DESC_TABLE,
  SCF_BEGIN,        /// 事务开始语句，可以在这里扩展只读事务
  SCF_COMMIT,
  SCF_CLOG_SYNC,
  SCF_ROLLBACK,
  SCF_LOAD_DATA,
  SCF_HELP,
  SCF_EXIT,
  SCF_EXPLAIN,
};
/**
 * @brief 表示一个SQL语句
 * 
 */
class Command
{
public:
  enum SqlCommandFlag flag;
  Error error;
  Selects selection;
  Inserts insertion;
  Deletes deletion;
  Updates update;
  CreateTable create_table;
  DropTable drop_table;
  CreateIndex create_index;
  DropIndex drop_index;
  DescTable desc_table;
  LoadData load_data;
  Explain explain;

public:
  Command();
  explicit Command(SqlCommandFlag flag);
};

/**
 * @brief 表示语法解析后的数据
 * 叫ParsedSqlNode 可能会更清晰一点
 */
class ParsedSqlResult
{
public:
  void add_command(std::unique_ptr<Command> command);
  std::vector<std::unique_ptr<Command>> &commands()
  {
    return sql_commands_;
  }

private:
  std::vector<std::unique_ptr<Command>> sql_commands_;  /// 这里记录SQL命令。虽然看起来支持多个，但是当前仅处理一个
};
