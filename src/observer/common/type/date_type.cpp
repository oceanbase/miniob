/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */
#include <iomanip>

#include "common/lang/comparator.h"
#include "common/lang/sstream.h"
#include "common/log/log.h"
#include "common/type/date_type.h"
#include "common/value.h"

int DateType::compare(const Value &left, const Value &right) const
{
  return common::compare_int((void *)&left.value_.int_value_,(void *)&right.value_.int_value_);
}

RC DateType::to_string(const Value &val, string &result) const
{
  stringstream ss;
  ss << std::setw(4) << std::setfill('0') << val.value_.int_value_ / 10000
     << std::setw(2) << std::setfill('0') << (val.value_.int_value_ % 10000) / 100
     << std::setw(2) << std::setfill('0') << val.value_.int_value_ % 100;
  result = ss.str();
  return RC::SUCCESS;
}
