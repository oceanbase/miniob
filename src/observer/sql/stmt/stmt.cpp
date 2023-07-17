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
// Created by Wangyunlai on 2022/5/22.
//

#include "common/log/log.h"
#include "sql/stmt/stmt.h"
#include "sql/stmt/insert_stmt.h"
#include "sql/stmt/delete_stmt.h"
#include "sql/stmt/select_stmt.h"
#include "sql/stmt/explain_stmt.h"
#include "sql/stmt/create_index_stmt.h"
#include "sql/stmt/create_table_stmt.h"
#include "sql/stmt/desc_table_stmt.h"
#include "sql/stmt/help_stmt.h"
#include "sql/stmt/show_tables_stmt.h"
#include "sql/stmt/trx_begin_stmt.h"
#include "sql/stmt/trx_end_stmt.h"
#include "sql/stmt/exit_stmt.h"
#include "sql/stmt/set_variable_stmt.h"
#include "sql/stmt/load_data_stmt.h"

RC Stmt::create_stmt(Db *db, const Command &cmd, Stmt *&stmt)
{
  stmt = nullptr;

  switch (cmd.flag) {
    case SCF_INSERT: {
      return InsertStmt::create(db, cmd.insertion, stmt);
    }
    case SCF_DELETE: {
      return DeleteStmt::create(db, cmd.deletion, stmt);
    }
    case SCF_SELECT: {
      return SelectStmt::create(db, cmd.selection, stmt);
    }

    case SCF_EXPLAIN: {
      return ExplainStmt::create(db, cmd.explain, stmt);
    }

    case SCF_CREATE_INDEX: {
      return CreateIndexStmt::create(db, cmd.create_index, stmt);
    }

    case SCF_CREATE_TABLE: {
      return CreateTableStmt::create(db, cmd.create_table, stmt);
    }

    case SCF_DESC_TABLE: {
      return DescTableStmt::create(db, cmd.desc_table, stmt);
    }

    case SCF_HELP: {
      return HelpStmt::create(stmt);
    }

    case SCF_SHOW_TABLES: {
      return ShowTablesStmt::create(db, stmt);
    }

    case SCF_BEGIN: {
      return TrxBeginStmt::create(stmt);
    }

    case SCF_COMMIT:
    case SCF_ROLLBACK: {
      return TrxEndStmt::create(cmd.flag, stmt);
    }

    case SCF_EXIT: {
      return ExitStmt::create(stmt);
    }

    case SCF_SET_VARIABLE: {
      return SetVariableStmt::create(cmd.set_variable, stmt);
    }

    case SCF_LOAD_DATA: {
      return LoadDataStmt::create(db, cmd.load_data, stmt);
    }

    default: {
      LOG_INFO("Command::type %d doesn't need to create statement.", cmd.flag);
    } break;
  }
  return RC::UNIMPLENMENT;
}
