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

/**
 * @brief 尝试从Plan的缓存中获取Plan，如果没有命中，则执行Optimizer
 * @ingroup SQLStage
 * @details 实际上现在什么都没做。不过PlanCache对数据库的优化提升明显，是一个非常有趣的功能，
 * 感兴趣的同学可以参考OceanBase的实现
 */
class PlanCacheStage
{
};
