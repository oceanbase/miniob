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
// Created by Wangyunlai on 2021/5/12.
//

#include "session/session.h"
#include "storage/trx/trx.h"
#include "storage/db/db.h"
#include "storage/default/default_handler.h"
#include "common/global_context.h"

Session &Session::default_session()
{
  static Session session;
  return session;
}

Session::Session(const Session &other) : db_(other.db_)
{}

Session::~Session()
{
  if (nullptr != trx_) {
    GCTX.trx_kit_->destroy_trx(trx_);
    trx_ = nullptr;
  }
}

const char *Session::get_current_db_name() const
{
  if (db_ != nullptr)
    return db_->name();
  else
    return "";
}

Db *Session::get_current_db() const
{
  return db_;
}

void Session::set_current_db(const std::string &dbname)
{
  DefaultHandler &handler = DefaultHandler::get_default();
  Db *db = handler.find_db(dbname.c_str());
  if (db == nullptr) {
    LOG_WARN("no such database: %s", dbname.c_str());
    return;
  }

  LOG_TRACE("change db to %s", dbname.c_str());
  db_ = db;
}

void Session::set_trx_multi_operation_mode(bool multi_operation_mode)
{
  trx_multi_operation_mode_ = multi_operation_mode;
}

bool Session::is_trx_multi_operation_mode() const
{
  return trx_multi_operation_mode_;
}

Trx *Session::current_trx()
{
  if (trx_ == nullptr) {
    trx_ = GCTX.trx_kit_->create_trx(db_->clog_manager());
  }
  return trx_;
}

thread_local Session *thread_session = nullptr;

void Session::set_current_session(Session *session)
{
  thread_session = session;
}

Session *Session::current_session()
{
  return thread_session;
}

void Session::set_current_request(SessionEvent *request)
{
  current_request_ = request;
}

SessionEvent *Session::current_request() const
{
  return current_request_;
}
