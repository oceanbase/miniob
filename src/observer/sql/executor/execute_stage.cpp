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
// Created by Longda on 2021/4/13.
//

#include <string>
#include <sstream>
#include <memory>

#include "execute_stage.h"

#include "common/io/io.h"
#include "common/log/log.h"
#include "common/lang/defer.h"
#include "common/seda/timer_stage.h"
#include "common/lang/string.h"
#include "session/session.h"
#include "event/storage_event.h"
#include "event/sql_event.h"
#include "event/session_event.h"
#include "sql/expr/tuple.h"
#include "sql/operator/table_scan_physical_operator.h"
#include "sql/operator/index_scan_physical_operator.h"
#include "sql/operator/predicate_physical_operator.h"
#include "sql/operator/delete_physical_operator.h"
#include "sql/operator/project_physical_operator.h"
#include "sql/operator/string_list_physical_operator.h"
#include "sql/stmt/stmt.h"
#include "sql/stmt/select_stmt.h"
#include "sql/stmt/update_stmt.h"
#include "sql/stmt/delete_stmt.h"
#include "sql/stmt/insert_stmt.h"
#include "sql/stmt/filter_stmt.h"
#include "storage/common/table.h"
#include "storage/common/field.h"
#include "storage/index/index.h"
#include "storage/default/default_handler.h"
#include "storage/common/condition_filter.h"
#include "storage/trx/trx.h"
#include "sql/executor/command_executor.h"

using namespace std;
using namespace common;

//! Constructor
ExecuteStage::ExecuteStage(const char *tag) : Stage(tag)
{}

//! Destructor
ExecuteStage::~ExecuteStage()
{}

//! Parse properties, instantiate a stage object
Stage *ExecuteStage::make_stage(const std::string &tag)
{
  ExecuteStage *stage = new (std::nothrow) ExecuteStage(tag.c_str());
  if (stage == nullptr) {
    LOG_ERROR("new ExecuteStage failed");
    return nullptr;
  }
  stage->set_properties();
  return stage;
}

//! Set properties for this object set in stage specific properties
bool ExecuteStage::set_properties()
{
  //  std::string stageNameStr(stageName);
  //  std::map<std::string, std::string> section = theGlobalProperties()->get(
  //    stageNameStr);
  //
  //  std::map<std::string, std::string>::iterator it;
  //
  //  std::string key;

  return true;
}

//! Initialize stage params and validate outputs
bool ExecuteStage::initialize()
{
  LOG_TRACE("Enter");

  std::list<Stage *>::iterator stgp = next_stage_list_.begin();
  default_storage_stage_ = *(stgp++);
  mem_storage_stage_ = *(stgp++);

  LOG_TRACE("Exit");
  return true;
}

//! Cleanup after disconnection
void ExecuteStage::cleanup()
{
  LOG_TRACE("Enter");

  LOG_TRACE("Exit");
}

void ExecuteStage::handle_event(StageEvent *event)
{
  LOG_TRACE("Enter");

  handle_request(event);

  LOG_TRACE("Exit");
  return;
}

void ExecuteStage::callback_event(StageEvent *event, CallbackContext *context)
{
  LOG_TRACE("Enter");

  // here finish read all data from disk or network, but do nothing here.

  LOG_TRACE("Exit");
  return;
}

RC ExecuteStage::handle_request(common::StageEvent *event)
{
  SQLStageEvent *sql_event = static_cast<SQLStageEvent *>(event);
  SqlResult *sql_result = sql_event->session_event()->sql_result();

  const unique_ptr<PhysicalOperator> &physical_operator = sql_event->physical_operator();
  if (physical_operator != nullptr) {
    return handle_request_with_physical_operator(sql_event);
  }

  SessionEvent *session_event = sql_event->session_event();
  Session *session = session_event->session();

  Command *sql = sql_event->command().get();
  Stmt *stmt = sql_event->stmt();
  if (stmt != nullptr) {
    CommandExecutor command_executor;
    RC rc = RC::UNIMPLENMENT;
    switch (stmt->type()) {
      case StmtType::UPDATE: {
        // do_update((UpdateStmt *)stmt, session_event);
      } break;
      case StmtType::CREATE_INDEX: {
        rc = command_executor.execute(session, stmt);
      } break;
      default: {
        LOG_WARN("should not happen. please implement this type:%d", stmt->type());
      } break;
    }
    session_event->sql_result()->set_return_code(rc);
  } else {
    // TODO create command to execute these sql
    switch (sql->flag) {
      case SCF_HELP: {
        do_help(sql_event);
      } break;
      case SCF_CREATE_TABLE: {
        do_create_table(sql_event);
      } break;

      case SCF_SHOW_TABLES: {
        do_show_tables(sql_event);
      } break;
      case SCF_DESC_TABLE: {
        do_desc_table(sql_event);
      } break;

      case SCF_DROP_TABLE:
      case SCF_DROP_INDEX:
      case SCF_LOAD_DATA: {
        default_storage_stage_->handle_event(event);
      } break;
      case SCF_BEGIN: {
        do_begin(sql_event);
      } break;
      case SCF_COMMIT: {
        do_commit(sql_event);
      } break;

      case SCF_ROLLBACK: {
        Trx *trx = session_event->session()->current_trx();
        RC rc = trx->rollback();
        session->set_trx_multi_operation_mode(false);

        sql_result->set_return_code(rc);
      } break;

      case SCF_EXIT: {
        // do nothing
        sql_result->set_return_code(RC::SUCCESS);
      } break;

      default: {
        LOG_ERROR("Unsupported command=%d\n", sql->flag);

        sql_result->set_return_code(RC::UNIMPLENMENT);
        sql_result->set_state_string("Unsupported command");
      }
    }
  }
  return RC::SUCCESS;
}

RC ExecuteStage::handle_request_with_physical_operator(SQLStageEvent *sql_event)
{
  RC rc = RC::SUCCESS;

  Stmt *stmt = sql_event->stmt();
  ASSERT(stmt != nullptr, "SQL Statement shouldn't be empty!");

  unique_ptr<PhysicalOperator> &physical_operator = sql_event->physical_operator();
  ASSERT(physical_operator != nullptr, "physical operator should not be null");

  TupleSchema schema;
  switch (stmt->type()) {
    case StmtType::SELECT: {
      SelectStmt *select_stmt = static_cast<SelectStmt *>(stmt);
      bool with_table_name = select_stmt->tables().size() > 1;

      for (const Field &field : select_stmt->query_fields()) {
        if (with_table_name) {
          schema.append_cell(field.table_name(), field.field_name());
        } else {
          schema.append_cell(field.field_name());
        }
      }
    } break;

    case StmtType::EXPLAIN: {
      schema.append_cell("Query Plan");
    } break;
    default: {
      // 只有select返回结果
    } break;
  }

  SqlResult *sql_result = sql_event->session_event()->sql_result();
  sql_result->set_tuple_schema(schema);
  sql_result->set_operator(std::move(physical_operator));
  return rc;
}

void end_trx_if_need(Session *session, Trx *trx, bool all_right)
{
  if (!session->is_trx_multi_operation_mode()) {
    if (all_right) {
      trx->commit();
    } else {
      trx->rollback();
    }
  }
}

RC ExecuteStage::do_help(SQLStageEvent *sql_event)
{
  const char *strings[] = {"show tables;",
      "desc `table name`;",
      "create table `table name` (`column name` `column type`, ...);",
      "create index `index name` on `table` (`column`);",
      "insert into `table` values(`value1`,`value2`);",
      "update `table` set column=value [where `column`=`value`];",
      "delete from `table` [where `column`=`value`];",
      "select [ * | `columns` ] from `table`;"};

  auto oper = new StringListPhysicalOperator();
  for (size_t i = 0; i < sizeof(strings) / sizeof(strings[0]); i++) {
    oper->append(strings[i]);
  }

  SqlResult *sql_result = sql_event->session_event()->sql_result();

  TupleSchema schema;
  schema.append_cell("Commands");

  sql_result->set_tuple_schema(schema);
  sql_result->set_operator(unique_ptr<PhysicalOperator>(oper));

  return RC::SUCCESS;
}

RC ExecuteStage::do_create_table(SQLStageEvent *sql_event)
{
  SessionEvent *session_event = sql_event->session_event();
  Db *db = session_event->session()->get_current_db();

  const CreateTable &create_table = sql_event->command()->create_table;
  const int attribute_count = static_cast<int>(create_table.attr_infos.size());

  RC rc = db->create_table(create_table.relation_name.c_str(), attribute_count, create_table.attr_infos.data());

  SqlResult *sql_result = session_event->sql_result();
  sql_result->set_return_code(rc);
  return rc;
}

RC ExecuteStage::do_show_tables(SQLStageEvent *sql_event)
{
  SqlResult *sql_result = sql_event->session_event()->sql_result();
  SessionEvent *session_event = sql_event->session_event();

  Db *db = session_event->session()->get_current_db();

  std::vector<std::string> all_tables;
  db->all_tables(all_tables);

  TupleSchema tuple_schema;
  tuple_schema.append_cell(TupleCellSpec("", "Tables_in_SYS", "Tables_in_SYS"));
  sql_result->set_tuple_schema(tuple_schema);

  auto oper = new StringListPhysicalOperator;
  for (const std::string &s : all_tables) {
    oper->append(s);
  }

  sql_result->set_operator(std::unique_ptr<PhysicalOperator>(oper));
  return RC::SUCCESS;
}

RC ExecuteStage::do_desc_table(SQLStageEvent *sql_event)
{
  SqlResult *sql_result = sql_event->session_event()->sql_result();

  Command *cmd = sql_event->command().get();
  const char *table_name = cmd->desc_table.relation_name.c_str();

  Db *db = sql_event->session_event()->session()->get_current_db();
  Table *table = db->find_table(table_name);
  if (table != nullptr) {

    TupleSchema tuple_schema;
    tuple_schema.append_cell(TupleCellSpec("", "Field", "Field"));
    tuple_schema.append_cell(TupleCellSpec("", "Type", "Type"));
    tuple_schema.append_cell(TupleCellSpec("", "Length", "Length"));

    // TODO add Key
    sql_result->set_tuple_schema(tuple_schema);

    auto oper = new StringListPhysicalOperator;
    const TableMeta &table_meta = table->table_meta();
    for (int i = table_meta.sys_field_num(); i < table_meta.field_num(); i++) {
      const FieldMeta *field_meta = table_meta.field(i);
      oper->append({field_meta->name(), attr_type_to_string(field_meta->type()), std::to_string(field_meta->len())});
    }

    sql_result->set_operator(unique_ptr<PhysicalOperator>(oper));
  } else {

    sql_result->set_return_code(RC::SCHEMA_TABLE_NOT_EXIST);
    sql_result->set_state_string("Table not exists");
  }
  return RC::SUCCESS;
}

RC ExecuteStage::do_begin(SQLStageEvent *sql_event)
{
  SessionEvent *session_event = sql_event->session_event();
  SqlResult *sql_result = session_event->sql_result();

  Session *session = session_event->session();
  Trx *trx = session->current_trx();

  session->set_trx_multi_operation_mode(true);

  RC rc = trx->start_if_need();
  sql_result->set_return_code(rc);

  return rc;
}

RC ExecuteStage::do_commit(SQLStageEvent *sql_event)
{
  SessionEvent *session_event = sql_event->session_event();
  SqlResult *sql_result = session_event->sql_result();

  Session *session = session_event->session();
  session->set_trx_multi_operation_mode(false);

  Trx *trx = session->current_trx();

  RC rc = trx->commit();
  sql_result->set_return_code(rc);

  return rc;
}
