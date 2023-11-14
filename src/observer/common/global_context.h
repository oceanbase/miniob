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
// Created by Wangyunlai on 2023/5/29.
//

#pragma once

class BufferPoolManager;
class DefaultHandler;
class TrxKit;

/**
 * @brief 放一些全局对象
 * @details 为了更好的管理全局对象，这里将其封装到一个类中。初始化的过程可以参考 init_global_objects
 */
struct GlobalContext
{
  BufferPoolManager *buffer_pool_manager_ = nullptr;
  DefaultHandler    *handler_             = nullptr;
  TrxKit            *trx_kit_             = nullptr;

  static GlobalContext &instance();
};

#define GCTX GlobalContext::instance()