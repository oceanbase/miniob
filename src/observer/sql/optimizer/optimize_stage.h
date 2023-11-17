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
// Created by Longda on 2021/4/13.
//

#pragma once

#include "common/rc.h"
#include "sql/operator/logical_operator.h"
#include "sql/operator/physical_operator.h"
#include "sql/optimizer/logical_plan_generator.h"
#include "sql/optimizer/physical_plan_generator.h"
#include "sql/optimizer/rewriter.h"

class SQLStageEvent;
class LogicalOperator;
class Stmt;

/**
 * @brief 将解析后的Statement转换成执行计划，并进行优化
 * @ingroup SQLStage
 * @details 优化分为两种，一个是根据规则重写，一个是根据代价模型优化。
 * 在这里，先将Statement转换成逻辑计划，然后进行重写(rewrite)，最后生成物理计划。
 * 不过并不是所有的语句都需要生成计划，有些可以直接执行，比如create table、create index等。
 * 这些语句可以参考 @class CommandExecutor。
 */
class OptimizeStage
{
public:
  RC handle_request(SQLStageEvent *event);

private:
  /**
   * @brief 根据SQL生成逻辑计划
   * @details
   * 由于SQL语句种类比较多，并且SQL语句可能会有嵌套的情况，比如带有SQL子查询的语句，那就需要递归的创建逻辑计划。
   * @param sql_event   包含SQL信息的事件
   * @param logical_operator  生成的逻辑计划
   */
  RC create_logical_plan(SQLStageEvent *sql_event, std::unique_ptr<LogicalOperator> &logical_operator);

  /**
   * @brief 重写逻辑计划
   * @details 根据各种规则，对逻辑计划进行重写，比如消除多余的比较(1!=0)等。
   * 规则改写也是一个递归的过程。
   * @param logical_operator 要改写的逻辑计划
   */
  RC rewrite(std::unique_ptr<LogicalOperator> &logical_operator);

  /**
   * @brief 优化逻辑计划
   * @details 当前什么都没做。可以增加每个逻辑计划的代价模型，然后根据代价模型进行优化。
   * @param logical_operator 需要优化的逻辑计划
   */
  RC optimize(std::unique_ptr<LogicalOperator> &logical_operator);

  /**
   * @brief 根据逻辑计划生成物理计划
   * @details 生成的物理计划就可以直接让后面的执行器完全按照物理计划执行了。
   * 物理计划与逻辑计划有些不同，逻辑计划描述要干什么，比如从某张表根据什么条件获取什么数据。
   * 而物理计划描述怎么做，比如如何从某张表按照什么条件获取什么数据，是否使用索引，使用哪个索引等。
   * @param physical_operator 生成的物理计划。通常是一个多叉树的形状，这里就拿着根节点就可以了。
   */
  RC generate_physical_plan(
      std::unique_ptr<LogicalOperator> &logical_operator, std::unique_ptr<PhysicalOperator> &physical_operator);

private:
  LogicalPlanGenerator  logical_plan_generator_;   ///< 根据SQL生成逻辑计划
  PhysicalPlanGenerator physical_plan_generator_;  ///< 根据逻辑计划生成物理计划
  Rewriter              rewriter_;                 ///< 逻辑计划改写
};
