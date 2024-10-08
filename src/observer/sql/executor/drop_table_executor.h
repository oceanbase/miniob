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
// Created by Floyd on 2024/10/07.
//

#pragma once

#ifndef DROP_TABLE_EXECUTOR_H_
#define DROP_TABLE_EXECUTOR_H_

#include "common/rc.h"

class SQLStageEvent;

/**
** @brief 删除表的执行器
** @ingroup Executor
*/
class DropTableExecutor
{
public:
    DropTableExecutor() = default;
    virtual ~DropTableExecutor() = default;

    RC execute(SQLStageEvent *sql_event);
};

#endif // DROP_TABLE_EXECUTOR_H_
