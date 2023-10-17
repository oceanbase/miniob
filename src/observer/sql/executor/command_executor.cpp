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
// Created by Wangyunlai on 2023/4/25.
//

#include "sql/executor/command_executor.h"
#include "event/sql_event.h"
#include "sql/stmt/stmt.h"
#include "sql/executor/create_index_executor.h"
#include "sql/executor/create_table_executor.h"
#include "sql/executor/desc_table_executor.h"
#include "sql/executor/help_executor.h"
#include "sql/executor/show_tables_executor.h"
#include "sql/executor/trx_begin_executor.h"
#include "sql/executor/trx_end_executor.h"
#include "sql/executor/set_variable_executor.h"
#include "sql/executor/load_data_executor.h"
#include "common/log/log.h"

RC CommandExecutor::execute(SQLStageEvent *sql_event)
{
  Stmt *stmt = sql_event->stmt();

  switch (stmt->type()) {
    case StmtType::CREATE_INDEX: {
      CreateIndexExecutor executor;
      return executor.execute(sql_event);
    } break;

    case StmtType::CREATE_TABLE: {
      CreateTableExecutor executor;
      return executor.execute(sql_event);
    } break;

    case StmtType::DESC_TABLE: {
      DescTableExecutor executor;
      return executor.execute(sql_event);
    }

    case StmtType::HELP: {
      HelpExecutor executor;
      return executor.execute(sql_event);
    }

    case StmtType::SHOW_TABLES: {
      ShowTablesExecutor executor;
      return executor.execute(sql_event);
    }

    case StmtType::BEGIN: {
      TrxBeginExecutor executor;
      return executor.execute(sql_event);
    }

    case StmtType::COMMIT:
    case StmtType::ROLLBACK: {
      TrxEndExecutor executor;
      return executor.execute(sql_event);
    }

    case StmtType::SET_VARIABLE: {
      SetVariableExecutor executor;
      return executor.execute(sql_event);
    }

    case StmtType::LOAD_DATA: {
      LoadDataExecutor executor;
      return executor.execute(sql_event);
    }

    case StmtType::EXIT: {
      return RC::SUCCESS;
    }

    default: {
      LOG_ERROR("unknown command: %d", static_cast<int>(stmt->type()));
      return RC::UNIMPLENMENT;
    }
  }

  return RC::INTERNAL;
}
