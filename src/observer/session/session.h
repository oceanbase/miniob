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

#pragma once

#include "common/types.h"
#include "common/lang/string.h"

class Trx;
class Db;
class SessionEvent;

/**
 * @brief 表示会话
 * @details 当前一个连接一个会话，没有做特殊的会话管理，这也简化了会话处理
 */
class Session
{
public:
  /**
   * @brief 获取默认的会话数据，新生成的会话都基于默认会话设置参数
   * @note 当前并没有会话参数
   */
  static Session &default_session();

public:
  Session() = default;
  ~Session();

  Session(const Session &other);
  void operator=(Session &) = delete;

  const char *get_current_db_name() const;
  Db         *get_current_db() const;

  /**
   * @brief 设置当前会话关联的数据库
   *
   * @param dbname 数据库名字
   */
  void set_current_db(const string &dbname);

  /**
   * @brief 设置当前事务为多语句模式，需要明确的指出提交或回滚
   */
  void set_trx_multi_operation_mode(bool multi_operation_mode);

  /**
   * @brief 当前事务是否为多语句模式
   */
  bool is_trx_multi_operation_mode() const;

  /**
   * @brief 当前会话关联的事务
   *
   */
  Trx *current_trx();

  /**
   * @brief 设置当前正在处理的请求
   */
  void set_current_request(SessionEvent *request);

  /**
   * @brief 获取当前正在处理的请求
   */
  SessionEvent *current_request() const;

  void set_sql_debug(bool sql_debug) { sql_debug_ = sql_debug; }
  bool sql_debug_on() const { return sql_debug_; }

  void          set_execution_mode(const ExecutionMode mode) { execution_mode_ = mode; }
  ExecutionMode get_execution_mode() const { return execution_mode_; }

  bool used_chunk_mode() { return used_chunk_mode_; }

  void set_used_chunk_mode(bool used_chunk_mode) { used_chunk_mode_ = used_chunk_mode; }

  /**
   * @brief 将指定会话设置到线程变量中
   *
   */
  static void set_current_session(Session *session);

  /**
   * @brief 获取当前的会话
   * @details 当前某个请求开始时，会将会话设置到线程变量中，在整个请求处理过程中不会改变
   */
  static Session *current_session();

private:
  Db           *db_              = nullptr;
  Trx          *trx_             = nullptr;
  SessionEvent *current_request_ = nullptr;  ///< 当前正在处理的请求

  bool trx_multi_operation_mode_ = false;  ///< 当前事务的模式，是否多语句模式. 单语句模式自动提交

  bool sql_debug_ = false;  ///< 是否输出SQL调试信息

  // 是否使用了 `chunk_iterator` 模式。 只有在设置了 `chunk_iterator`
  // 并且可以生成相关物理执行计划时才会使用 `chunk_iterator` 模式。
  bool used_chunk_mode_ = false;

  ExecutionMode execution_mode_ = ExecutionMode::TUPLE_ITERATOR;
};
