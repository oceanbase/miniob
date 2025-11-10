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
// Created by Wangyunlai on 2024/01/10.
//

#include "net/sql_task_handler.h"
#include "net/communicator.h"
#include "event/session_event.h"
#include "event/sql_event.h"
#include "session/session.h"

RC SqlTaskHandler::handle_event(Communicator *communicator)
{
  SessionEvent *event = nullptr;
  RC            rc    = communicator->read_event(event);
  if (OB_FAIL(rc)) {
    return rc;
  }

  if (nullptr == event) {
    return RC::SUCCESS;
  }

  session_stage_.handle_request(event);

  SQLStageEvent sql_event(event, event->query());

  rc = handle_sql(&sql_event);
  if (OB_FAIL(rc)) {
    LOG_TRACE("failed to handle sql. rc=%s", strrc(rc));
    event->sql_result()->set_return_code(rc);
  }

  bool need_disconnect = false;

  rc = communicator->write_result(event, need_disconnect);
  LOG_INFO("write result return %s", strrc(rc));
  event->session()->set_current_request(nullptr);
  Session::set_current_session(nullptr);

  delete event;

  if (need_disconnect) {
    return RC::INTERNAL;
  }
  return RC::SUCCESS;
}

/**
 * 处理一个SQL语句经历这几个阶段。
 * 虽然看起来流程比较多，但是对于大多数SQL来说，更多的可以关注parse和executor阶段。
 * 通常只有select、delete等带有查询条件的语句才需要进入optimize。
 * 对于DDL语句，比如create table、create index等，没有对应的查询计划，可以直接搜索
 * create_table_executor、create_index_executor来看具体的执行代码。
 * select、delete等DML语句，会产生一些执行计划，如果感觉繁琐，可以跳过optimize直接看
 * execute_stage中的执行，通过explain语句看需要哪些operator，然后找对应的operator来
 * 调试或者看代码执行过程即可。
 */
RC SqlTaskHandler::handle_sql(SQLStageEvent *sql_event)
{
  RC rc = query_cache_stage_.handle_request(sql_event);
  if (OB_FAIL(rc)) {
    LOG_TRACE("failed to do query cache. rc=%s", strrc(rc));
    return rc;
  }

  rc = parse_stage_.handle_request(sql_event);
  if (OB_FAIL(rc)) {
    LOG_TRACE("failed to do parse. rc=%s", strrc(rc));
    return rc;
  }

  rc = resolve_stage_.handle_request(sql_event);
  if (OB_FAIL(rc)) {
    LOG_TRACE("failed to do resolve. rc=%s", strrc(rc));
    return rc;
  }

  rc = optimize_stage_.handle_request(sql_event);
  if (rc != RC::UNIMPLEMENTED && rc != RC::SUCCESS) {
    LOG_TRACE("failed to do optimize. rc=%s", strrc(rc));
    return rc;
  }

  rc = execute_stage_.handle_request(sql_event);
  if (OB_FAIL(rc)) {
    LOG_TRACE("failed to do execute. rc=%s", strrc(rc));
    return rc;
  }

  return rc;
}