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

class SQLStageEvent;

/**
 * @brief 查询缓存处理
 * @ingroup SQLStage
 * @details 当前什么都没做
 */
class QueryCacheStage
{
public:
  QueryCacheStage() = default;
  virtual ~QueryCacheStage() = default;

public:
  RC handle_request(SQLStageEvent *sql_event);
};
