/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
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

#ifndef __OBSERVER_SESSION_SESSION_H__
#define __OBSERVER_SESSION_SESSION_H__

#include <string>

class Trx;

class Session {
public:
  // static Session &current();
  static Session &default_session();

public:
  Session() = default;
  ~Session();

  Session(const Session &other);
  void operator=(Session &) = delete;

  const std::string &get_current_db() const;
  void set_current_db(const std::string &dbname);

  void set_trx_multi_operation_mode(bool multi_operation_mode);
  bool is_trx_multi_operation_mode() const;

  Trx *current_trx();

private:
  std::string current_db_;
  Trx *trx_ = nullptr;
  bool trx_multi_operation_mode_ = false;  // 当前事务的模式，是否多语句模式. 单语句模式自动提交
};

#endif  // __OBSERVER_SESSION_SESSION_H__