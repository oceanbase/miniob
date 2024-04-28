/* Copyright (c) 2021-2022 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by wangyunlai on 2024/01/30
//

#pragma once

#include <stdint.h>

/**
 * @brief 日志模块
 * @ingroup CLog
 */
class LogModule
{
public:
  enum class Id
  {
    BUFFER_POOL,     /// 缓冲池
    BPLUS_TREE,      /// B+树
    RECORD_MANAGER,  /// 记录管理
    TRANSACTION      /// 事务
  };

public:
  explicit LogModule(Id id) : id_(id) {}
  explicit LogModule(int32_t id) : id_(static_cast<Id>(id)) {}

  Id      id() const { return id_; }
  int32_t index() const { return static_cast<int32_t>(id_); }

  const char *name() const
  {
    switch (id_) {
      case Id::BUFFER_POOL: return "BUFFER_POOL";
      case Id::BPLUS_TREE: return "BPLUS_TREE";
      case Id::RECORD_MANAGER: return "RECORD_MANAGER";
      case Id::TRANSACTION: return "TRANSACTION";
      default: return "UNKNOWN";
    }
  }

private:
  Id id_;
};