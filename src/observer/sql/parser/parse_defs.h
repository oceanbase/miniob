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
//

#pragma once

#include <stddef.h>
#include <memory>
#include <vector>
#include <string>

#define MAX_NUM 20
#define MAX_REL_NAME 20
#define MAX_ATTR_NAME 20
#define MAX_ERROR_MESSAGE 20
#define MAX_DATA 50

// 属性结构体
struct RelAttr {
  std::string relation_name;   // relation name (may be NULL) 表名
  std::string attribute_name;  // attribute name              属性名
};

enum CompOp {
  EQUAL_TO,     //"="     0
  LESS_EQUAL,   //"<="    1
  NOT_EQUAL,    //"<>"    2
  LESS_THAN,    //"<"     3
  GREAT_EQUAL,  //">="    4
  GREAT_THAN,   //">"     5
  NO_OP
};

// 属性值类型
enum AttrType {
  UNDEFINED,
  CHARS,
  INTS,
  FLOATS,
  BOOLEANS,
};

// 属性值
struct Value {
  AttrType type;  // type of value
  int int_value;
  float float_value;
  bool bool_value;
  std::string string_value;

  const char *data() const;
  int length();
};

struct Condition {
  int left_is_attr;    // TRUE if left-hand side is an attribute
                       // 1时，操作符左边是属性名，0时，是属性值
  Value left_value;    // left-hand side value if left_is_attr = FALSE
  RelAttr left_attr;   // left-hand side attribute
  CompOp comp;         // comparison operator
  int right_is_attr;   // TRUE if right-hand side is an attribute
                       // 1时，操作符右边是属性名，0时，是属性值
  RelAttr right_attr;  // right-hand side attribute if right_is_attr = TRUE 右边的属性
  Value right_value;   // right-hand side value if right_is_attr = FALSE
};

// struct of select
struct Selects {
  std::vector<RelAttr> attributes;  // attributes in select clause
  std::vector<std::string> relations;
  std::vector<Condition> conditions;
};

// struct of insert
struct Inserts {
  std::string relation_name;  // Relation to insert into
  std::vector<Value> values;
};

// struct of delete
struct Deletes {
  std::string relation_name;  // Relation to delete from
  std::vector<Condition> conditions;
};

// struct of update
struct Updates {
  std::string relation_name;   // Relation to update
  std::string attribute_name;  // Attribute to update
  Value value;                 // update value
  std::vector<Condition> conditions;
};

struct AttrInfo {
  AttrType type;     // Type of attribute
  std::string name;  // Attribute name
  size_t length;     // Length of attribute
};

// struct of craete_table
struct CreateTable {
  std::string relation_name;         // Relation name
  std::vector<AttrInfo> attr_infos;  // attributes
};

// struct of drop_table
struct DropTable {
  std::string relation_name;  // Relation name
};

// struct of create_index
struct CreateIndex {
  std::string index_name;      // Index name
  std::string relation_name;   // Relation name
  std::string attribute_name;  // Attribute name
};

// struct of  drop_index
struct DropIndex {
  std::string index_name;     // Index name
  std::string relation_name;  // Relation name
};

struct DescTable {
  std::string relation_name;
};

struct LoadData {
  std::string relation_name;
  std::string file_name;
};

class Command;
struct Explain {
  std::unique_ptr<Command> cmd;
};

struct Error {
  std::string error_msg;
  int line;
  int column;
};

// 修改yacc中相关数字编码为宏定义
enum SqlCommandFlag {
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
  SCF_BEGIN,
  SCF_COMMIT,
  SCF_CLOG_SYNC,
  SCF_ROLLBACK,
  SCF_LOAD_DATA,
  SCF_HELP,
  SCF_EXIT,
  SCF_EXPLAIN,
};
// struct of flag and sql_struct
class Command {
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
  Command(enum SqlCommandFlag flag);
};

/**
 * 表示语法解析后的数据
 * 叫ParsedSqlNode 可能会更清晰一点
 */
class ParsedSqlResult {
public:
  void add_command(std::unique_ptr<Command> command);
  std::vector<std::unique_ptr<Command>> &commands()
  {
    return sql_commands_;
  }

private:
  std::vector<std::unique_ptr<Command>> sql_commands_;
};

const char *attr_type_to_string(AttrType type);
AttrType attr_type_from_string(const char *s);
