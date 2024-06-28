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
// Created by Wangyunlai on 2024/01/10.
//

#include "net/thread_handler.h"
#include "common/lang/mutex.h"
#include "common/lang/unordered_map.h"

class Worker;

/**
 * @brief 一个连接一个线程的线程模型
 * @ingroup ThreadHandler
 */
class OneThreadPerConnectionThreadHandler : public ThreadHandler
{
public:
  OneThreadPerConnectionThreadHandler() = default;
  virtual ~OneThreadPerConnectionThreadHandler();

  //! @copydoc ThreadHandler::start
  virtual RC start() override { return RC::SUCCESS; }

  //! @copydoc ThreadHandler::stop
  virtual RC stop() override;
  //! @copydoc ThreadHandler::await_stop
  virtual RC await_stop() override;

  //! @copydoc ThreadHandler::new_connection
  virtual RC new_connection(Communicator *communicator) override;
  //! @copydoc ThreadHandler::close_connection
  virtual RC close_connection(Communicator *communicator) override;

private:
  /// 记录一个连接Communicator关联的线程数据
  unordered_map<Communicator *, Worker *> thread_map_;  // 当前编译器没有支持jthread
  /// 保护线程安全的锁
  mutex lock_;
};