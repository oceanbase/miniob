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
// Created by Wangyunlai on 2022/11/22.
//

#pragma once

#include "net/communicator.h"

class SqlResult;
class BasePacket;

/**
 * @brief 与客户端通讯
 * @ingroup Communicator
 * @ingroup MySQLProtol
 * @details 实现MySQL通讯协议
 * 可以参考 [MySQL Page Protocol](https://dev.mysql.com/doc/dev/mysql-server/latest/PAGE_PROTOCOL.html)
 * 或 [MariaDB Protocol](https://mariadb.com/kb/en/clientserver-protocol/)
 */
class MysqlCommunicator : public Communicator
{
public:
  /**
   * @brief 连接刚开始建立时，进行一些初始化
   * @details 参考MySQL或MariaDB的手册，服务端要首先向客户端发送一个握手包，等客户端回复后，
   * 再回复一个OkPacket或ErrPacket
   */
  virtual RC init(int fd, Session *session, const std::string &addr) override;

  /**
   * @brief 有新的消息到达时，接收消息
   * @details 因为MySQL协议的特殊性，收到数据后不一定需要向后流转，比如握手包
   */
  virtual RC read_event(SessionEvent *&event) override;

  /**
   * @brief 将处理结果返回给客户端
   * @param[in] event 任务数据，包括处理的结果
   * @param[out] need_disconnect 是否需要断开连接
   */
  virtual RC write_result(SessionEvent *event, bool &need_disconnect) override;

private:
  /**
   * @brief 发送数据包到客户端
   *
   * @param[in] packet 要发送的数据包
   */
  RC send_packet(const BasePacket &packet);

  /**
   * @brief 有些情况下不需要给客户端返回一行行的记录结果，而是返回执行是否成功即可，比如create table等
   *
   * @param[in] event 处理的结果
   * @param[out] need_disconnect 是否需要断开连接
   */
  RC write_state(SessionEvent *event, bool &need_disconnect);

  /**
   * @brief 返回客户端列描述信息
   * @details 根据MySQL text protocol 描述，普通的结果分为列信息描述和行数据。
   * 这里就分为两个函数
   */
  RC send_column_definition(SqlResult *sql_result, bool &need_disconnect);

  /**
   * @brief 返回客户端行数据
   *
   * @param[in] sql_result 返回的结果
   * @param no_column_def 是否没有列描述信息
   * @param[out] need_disconnect 是否需要断开连接
   * @return RC
   */
  RC send_result_rows(SqlResult *sql_result, bool no_column_def, bool &need_disconnect);

  /**
   * @brief 根据实际测试，客户端在连接上来时，会发起一个 version_comment的查询
   * @details 这里就针对这个查询返回一个结果
   */
  RC handle_version_comment(bool &need_disconnect);

private:
  //! 握手阶段(鉴权)，需要做一些特殊处理，所以加个字段单独标记
  bool authed_ = false;

  //! 有时需要根据握手包中capabilities字段值的不同，而发送或接收不同的包
  uint32_t client_capabilities_flag_ = 0;

  //! 在一次通讯过程中(一个任务的请求与处理)，每个包(packet)都有一个sequence id
  //! 这个sequence id是递增的
  int8_t sequence_id_ = 0;
};
