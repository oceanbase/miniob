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
// Created by Floyd on 2024/10/07.
//

#pragma once

#ifndef DROP_TABLE_STMT_H_
#define DROP_TABLE_STMT_H_

#include <string>

#include "sql/stmt/stmt.h"

class Db;

/**
 * @brief 表示删除表的语句
 * @ingroup Statement
 */
class DropTableStmt : public Stmt
{
public:
    DropTableStmt(const std::string &table_name) : table_name_{table_name} {}
    virtual ~DropTableStmt() = default;

    StmtType type() const override { return StmtType::DROP_TABLE; }

    const std::string &table_name() const { return table_name_; }

    static RC create(Db *db, const DropTableSqlNode &drop_table, Stmt *&stmt);

private:
    std::string table_name_;
};

#endif // DROP_TABLE_STMT_H_
