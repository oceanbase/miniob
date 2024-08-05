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
#include "common/log/log.h"
#include "event/sql_event.h"
#include "sql/executor/create_index_executor.h"
#include "sql/executor/create_table_executor.h"
#include "sql/executor/desc_table_executor.h"
#include "sql/executor/help_executor.h"
#include "sql/executor/load_data_executor.h"
#include "sql/executor/set_variable_executor.h"
#include "sql/executor/show_tables_executor.h"
#include "sql/executor/trx_begin_executor.h"
#include "sql/executor/trx_end_executor.h"
#include "sql/stmt/stmt.h"

RC CommandExecutor::execute(SQLStageEvent *sql_event)
{
  Stmt *stmt = sql_event->stmt();

  RC rc = RC::SUCCESS;
  switch (stmt->type()) {
    case StmtType::CREATE_INDEX: {
      CreateIndexExecutor executor;
      rc = executor.execute(sql_event);
    } break;

    case StmtType::CREATE_TABLE: {
      CreateTableExecutor executor;
      rc = executor.execute(sql_event);
    } break;

    case StmtType::DESC_TABLE: {
      DescTableExecutor executor;
      rc = executor.execute(sql_event);
    } break;

    case StmtType::HELP: {
      HelpExecutor executor;
      rc = executor.execute(sql_event);
    } break;

    case StmtType::SHOW_TABLES: {
      ShowTablesExecutor executor;
      rc = executor.execute(sql_event);
    } break;

    case StmtType::BEGIN: {
      TrxBeginExecutor executor;
      rc = executor.execute(sql_event);
    } break;

    case StmtType::COMMIT:
    case StmtType::ROLLBACK: {
      TrxEndExecutor executor;
      rc = executor.execute(sql_event);
    } break;

    case StmtType::SET_VARIABLE: {
      SetVariableExecutor executor;
      rc = executor.execute(sql_event);
    } break;

    case StmtType::LOAD_DATA: {
      LoadDataExecutor executor;
      rc = executor.execute(sql_event);
    } break;

    case StmtType::EXIT: {
      rc = RC::SUCCESS;
    } break;

    default: {
      LOG_ERROR("unknown command: %d", static_cast<int>(stmt->type()));
      rc = RC::UNIMPLEMENTED;
    } break;
  }

  if (OB_SUCC(rc) && stmt_type_ddl(stmt->type())) {
    // 每次做完DDL之后，做一次sync，保证元数据与日志保持一致
    rc = sql_event->session_event()->session()->get_current_db()->sync();
    LOG_INFO("sync db after ddl. rc=%d", rc);
  }

  return rc;
}
