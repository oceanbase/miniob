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
// Created by Wangyunlai on 2022/5/22.
//

#pragma once

#include <string>
#include <vector>

#include "rc.h"
#include "sql/parser/parse_defs.h"
#include "sql/stmt/stmt.h"

class Table;

class UpdateStmt : public Stmt
{
public:
  UpdateStmt() = default;
  UpdateStmt(Table *table, const char *attribute_name, const Value &value, int condition_num,
      const Condition conditions[]);

  StmtType type() const override { return StmtType::UPDATE; }

public:
  static RC create(Db *db, const Updates &update, Stmt *&stmt);

public:
  Table *table() const { return table_; }
  const char *attribute_name() const { return attribute_name_.c_str(); }
  const Value &value() const { return value_; }
  int condition_num() const { return condition_num_; }
  const Condition *conditions() const { return conditions_; }

private:
  Table *table_ = nullptr;
  std::string attribute_name_;
  std::vector<char> value_buffer_;  // 更新值的深拷贝，避免解析树释放后悬空
  Value value_{};                   // 指向 value_buffer_
  int condition_num_ = 0;
  Condition conditions_[MAX_NUM];
};
