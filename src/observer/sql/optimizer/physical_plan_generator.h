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
// Created by Wangyunlai on 2022/12/14.
//

#pragma once

#include <memory>

#include "common/rc.h"
#include "sql/operator/physical_operator.h"
#include "sql/operator/logical_operator.h"

class TableGetLogicalOperator;
class PredicateLogicalOperator;
class ProjectLogicalOperator;
class InsertLogicalOperator;
class DeleteLogicalOperator;
class ExplainLogicalOperator;
class JoinLogicalOperator;
class CalcLogicalOperator;

/**
 * @brief 物理计划生成器
 * @ingroup PhysicalOperator
 * @details 根据逻辑计划生成物理计划。
 * 不会做任何优化，完全根据本意生成物理计划。
 */
class PhysicalPlanGenerator 
{
public:
  PhysicalPlanGenerator() = default;
  virtual ~PhysicalPlanGenerator() = default;

  RC create(LogicalOperator &logical_operator, std::unique_ptr<PhysicalOperator> &oper);

private:
  RC create_plan(TableGetLogicalOperator &logical_oper, std::unique_ptr<PhysicalOperator> &oper);
  RC create_plan(PredicateLogicalOperator &logical_oper, std::unique_ptr<PhysicalOperator> &oper);
  RC create_plan(ProjectLogicalOperator &logical_oper, std::unique_ptr<PhysicalOperator> &oper);
  RC create_plan(InsertLogicalOperator &logical_oper, std::unique_ptr<PhysicalOperator> &oper);
  RC create_plan(DeleteLogicalOperator &logical_oper, std::unique_ptr<PhysicalOperator> &oper);
  RC create_plan(ExplainLogicalOperator &logical_oper, std::unique_ptr<PhysicalOperator> &oper);
  RC create_plan(JoinLogicalOperator &logical_oper, std::unique_ptr<PhysicalOperator> &oper);
  RC create_plan(CalcLogicalOperator &logical_oper, std::unique_ptr<PhysicalOperator> &oper);
};
