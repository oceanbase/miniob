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
// Created by Wangyunlai on 2022/12/14.
//

#pragma once

#include <memory>

#include "rc.h"
#include "sql/operator/physical_operator.h"
#include "sql/operator/logical_operator.h"

class TableGetLogicalOperator;
class PredicateLogicalOperator;
class ProjectLogicalOperator;
class DeleteLogicalOperator;
class ExplainLogicalOperator;
class JoinLogicalOperator;

class PhysicalPlanGenerator {
public:
  PhysicalPlanGenerator() = default;
  virtual ~PhysicalPlanGenerator() = default;

  RC create(LogicalOperator &logical_operator, std::unique_ptr<PhysicalOperator> &oper);

private:
  RC create_plan(TableGetLogicalOperator &table_get_oper, std::unique_ptr<PhysicalOperator> &oper);
  RC create_plan(PredicateLogicalOperator &pred_oper, std::unique_ptr<PhysicalOperator> &oper);
  RC create_plan(ProjectLogicalOperator &project_oper, std::unique_ptr<PhysicalOperator> &oper);
  RC create_plan(DeleteLogicalOperator &delete_oper, std::unique_ptr<PhysicalOperator> &oper);
  RC create_plan(ExplainLogicalOperator &explain_oper, std::unique_ptr<PhysicalOperator> &oper);
  RC create_plan(JoinLogicalOperator &join_oper, std::unique_ptr<PhysicalOperator> &oper);
};
