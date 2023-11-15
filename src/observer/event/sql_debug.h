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
// Created by Wangyunlai on 2023/6/29.
//

#pragma once

#include <list>
#include <string>

/**
 * @brief SQL调试信息
 * @details
 * 希望在运行SQL时，可以直接输出一些调试信息到客户端。
 * 当前把调试信息都放在了session上，可以随着SQL语句输出。
 * 但是现在还不支持与输出调试信息与行数据同步输出。
 */
class SqlDebug
{
public:
  SqlDebug()          = default;
  virtual ~SqlDebug() = default;

  void add_debug_info(const std::string &debug_info);
  void clear_debug_info();

  const std::list<std::string> &get_debug_infos() const;

private:
  std::list<std::string> debug_infos_;
};

/**
 * @brief 增加SQL的调试信息
 * @details 可以在任何执行SQL语句时调用这个函数来增加调试信息。
 * 如果当前上下文不在SQL执行过程中，那么不会生成调试信息。
 * 在普通文本场景下，调试信息会直接输出到客户端，并增加 '#' 作为前缀。
 */
void sql_debug(const char *fmt, ...);
