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
// Created by WangYunlai on 2022/11/18.
//

#include "sql/operator/physical_operator.h"

std::string physical_operator_type_name(PhysicalOperatorType type)
{
  switch (type) {
    case PhysicalOperatorType::TABLE_SCAN:
      return "TABLE_SCAN";
    case PhysicalOperatorType::INDEX_SCAN:
      return "INDEX_SCAN";
    case PhysicalOperatorType::NESTED_LOOP_JOIN:
      return "NESTED_LOOP_JOIN";
    case PhysicalOperatorType::EXPLAIN:
      return "EXPLAIN";
    case PhysicalOperatorType::PREDICATE:
      return "PREDICATE";
    case PhysicalOperatorType::INSERT:
      return "INSERT";
    case PhysicalOperatorType::DELETE:
      return "DELETE";
    case PhysicalOperatorType::PROJECT:
      return "PROJECT";
    case PhysicalOperatorType::STRING_LIST:
      return "STRING_LIST";
    default:
      return "UNKNOWN";
  }
}

PhysicalOperator::~PhysicalOperator()
{}

std::string PhysicalOperator::name() const
{
  return physical_operator_type_name(type());
}

std::string PhysicalOperator::param() const
{
  return "";
}
