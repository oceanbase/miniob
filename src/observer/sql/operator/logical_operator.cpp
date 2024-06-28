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
// Created by Wangyunlai on 2022/12/08.
//

#include "sql/operator/logical_operator.h"

LogicalOperator::~LogicalOperator() {}

void LogicalOperator::add_child(std::unique_ptr<LogicalOperator> oper) { children_.emplace_back(std::move(oper)); }
bool LogicalOperator::can_generate_vectorized_operator(const LogicalOperatorType &type)
{
    bool bool_ret = false;
    switch (type)
    {
    case LogicalOperatorType::CALC:
    case LogicalOperatorType::DELETE:
    case LogicalOperatorType::INSERT:
        bool_ret = false;
        break;
    
    default:
        bool_ret = true;
        break;
    }
    return bool_ret;
}

