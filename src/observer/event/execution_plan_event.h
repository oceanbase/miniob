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
// Created by Wangyunlai on 2021/5/11.
//

#ifndef __OBSERVER_EVENT_EXECUTION_PLAN_EVENT_H__
#define __OBSERVER_EVENT_EXECUTION_PLAN_EVENT_H__

#include "common/seda/stage_event.h"
#include "sql/parser/parse.h"

class SQLStageEvent;

class ExecutionPlanEvent : public common::StageEvent {
public:
  ExecutionPlanEvent(SQLStageEvent *sql_event, Query *sqls);
  virtual ~ExecutionPlanEvent();

  Query * sqls() const {
    return sqls_;
  }

  SQLStageEvent * sql_event() const {
    return sql_event_;
  }
private:
  SQLStageEvent *      sql_event_;
  Query *             sqls_;
};

#endif // __OBSERVER_EVENT_EXECUTION_PLAN_EVENT_H__