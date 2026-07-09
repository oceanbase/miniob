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

#include <string.h>

#include "resolve_stage.h"

#include "common/conf/ini.h"
#include "common/io/io.h"
#include "common/lang/string.h"
#include "common/log/log.h"
#include "event/session_event.h"
#include "event/sql_event.h"
#include "session/session.h"
#include "sql/stmt/stmt.h"
#include "sql/parser/parse_defs.h" // 引入 ParsedSqlNode 定义

using namespace common;

RC ResolveStage::handle_request(SQLStageEvent *sql_event)
{
  RC            rc            = RC::SUCCESS;
  SessionEvent *session_event = sql_event->session_event();
  SqlResult    *sql_result    = session_event->sql_result();

  // 1. 提前获取 SQL Node
  ParsedSqlNode *sql_node = sql_event->sql_node().get();

  // 2.  拦截不需要数据库上下文或 Stmt 的系统命令
  // 这些命令在 ExecuteStage 中通过 switch(flag) 直接处理，因此在此处直接“放行”
  if (sql_node->flag == SCF_SHOW_VARIABLES || 
      sql_node->flag == SCF_SET_VARIABLE ||
      sql_node->flag == SCF_EXIT ||
      sql_node->flag == SCF_HELP) {
    return RC::SUCCESS;
  }

  // 3. 检查是否选择了数据库 (原有逻辑)
  Db *db = session_event->session()->get_current_db();
  if (nullptr == db) {
    // 这里需要注意：如果是创建数据库或显示数据库，也不需要当前有 DB
    // 但为了聚焦解决 SHOW VARIABLES 问题，且不破坏原有逻辑，我们只处理上面拦截的命令
    // 如果后续发现 CREATE DATABASE 报错，也需要加到上面的 if 中
    LOG_ERROR("cannot find current db");
    rc = RC::SCHEMA_DB_NOT_EXIST;
    sql_result->set_return_code(rc);
    sql_result->set_state_string("no db selected");
    return rc;
  }

  // 4. 创建 Stmt 
  Stmt *stmt = nullptr;
  rc = Stmt::create_stmt(db, *sql_node, stmt);
  if (rc != RC::SUCCESS && rc != RC::UNIMPLEMENTED) {
    LOG_WARN("failed to create stmt. rc=%d:%s", rc, strrc(rc));
    sql_result->set_return_code(rc);
    return rc;
  }

  sql_event->set_stmt(stmt);

  return rc;
}