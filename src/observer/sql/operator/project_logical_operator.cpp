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
// Created by Wangyunlai on 2022/12/15
//

#include "sql/operator/project_logical_operator.h"

using namespace std;

ProjectLogicalOperator::ProjectLogicalOperator(vector<unique_ptr<Expression>> &&expressions)
{
  expressions_ = std::move(expressions);
}

unique_ptr<LogicalProperty> ProjectLogicalOperator::find_log_prop(const vector<LogicalProperty*> &log_props)
{
  int card = 0;
  for (auto log_prop : log_props) {
    if (log_prop != nullptr) {
      card += log_prop->get_card();
    } else {
      LOG_WARN("find_log_prop: log_prop is nullptr");
    }
  }
  return make_unique<LogicalProperty>(card);
}
