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
// Created by Wangyunlai on 2023/06/25.
//

#pragma once

#include "net/plain_communicator.h"

/**
 * @brief 用于命令行模式的通讯器
 * @ingroup Communicator
 * @details 直接通过终端/标准输入输出进行通讯。从这里的实现来看，是不需要libevent的一些实现的，
 * 因此communicator需要重构，或者需要重构server中的各个通讯启动模式。
 */
class CliCommunicator : public PlainCommunicator
{
public:
  CliCommunicator() = default;
  virtual ~CliCommunicator() = default;

  RC init(int fd, Session *session, const std::string &addr) override;
  RC read_event(SessionEvent *&event) override;
  RC write_result(SessionEvent *event, bool &need_disconnect) override;

private:
  int write_fd_ = -1; ///< 与使用远程通讯模式不同，如果读数据使用标准输入，那么输出应该是标准输出
};
