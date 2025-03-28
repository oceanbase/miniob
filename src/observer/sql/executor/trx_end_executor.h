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
// Created by Wangyunlai on 2023/6/14.
//

#pragma once

#include "common/sys/rc.h"
#include "event/session_event.h"
#include "event/sql_event.h"
#include "session/session.h"
#include "sql/stmt/stmt.h"
#include "storage/trx/trx.h"

/**
 * @brief 事务结束的执行器，可以是提交或回滚
 * @ingroup Executor
 */
class TrxEndExecutor
{
public:
  TrxEndExecutor()          = default;
  virtual ~TrxEndExecutor() = default;

  RC execute(SQLStageEvent *sql_event)
  {
    RC            rc            = RC::SUCCESS;
    Stmt         *stmt          = sql_event->stmt();
    SessionEvent *session_event = sql_event->session_event();

    Session *session = session_event->session();
    session->set_trx_multi_operation_mode(false);
    Trx *trx = session->current_trx();

    if (stmt->type() == StmtType::COMMIT) {
      rc = trx->commit();
    } else {
      rc = trx->rollback();
    }
    session->destroy_trx();
    return rc;
  }
};
