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
// Created by Wangyunlai on 2022/6/7.
//

#pragma once

#include "common/seda/stage_event.h"

class SQLStageEvent;
class Stmt;

class OptimizeEvent : public common::StageEvent {
public:
  OptimizeEvent(SQLStageEvent *sql_event, common::StageEvent *parent_event)
    : sql_event_(sql_event), parent_event_(parent_event)
  {}

  virtual ~OptimizeEvent() noexcept = default;

  SQLStageEvent *sql_event() const {
    return sql_event_;
  }

  common::StageEvent *parent_event() const {
    return parent_event_;
  }

private:
  SQLStageEvent *sql_event_ = nullptr;
  common::StageEvent *parent_event_ = nullptr;
};

