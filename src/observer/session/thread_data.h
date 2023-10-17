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
// Created by Wangyunlai on 2023/03/07.
//

#pragma once

class Trx;
class Session;

class ThreadData
{
public:
  static ThreadData *current() { return thread_data_; }
  static void setup(ThreadData *thread) { thread_data_ = thread; }

public:
  ThreadData() = default;
  ~ThreadData() = default;

  Session *session() const { return session_; }
  Trx * trx() const;

  void set_session(Session *session) { session_ = session; }

private:
  static thread_local ThreadData * thread_data_;

private:
  Session *session_ = nullptr;
};