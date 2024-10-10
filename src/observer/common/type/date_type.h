/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#pragma once
#ifndef DATE_TYPE_H_
#define DATE_TYPE_H_

#include "common/rc.h"
#include "common/type/data_type.h"

/**
 * @brief 日期类型
 * @ingroup DataType
 */
class DateType : public DataType
{
public:
    DateType() : DataType(AttrType::DATES) {}

    virtual ~DateType() = default;

    int compare(const Value &left, const Value &right) const override;

    RC cast_to(const Value &val, AttrType type, Value &result) const override;

    int cast_cost(AttrType type) override;

    RC to_string(const Value &val, string &result) const override;
};

#endif // DATE_TYPE_H_
