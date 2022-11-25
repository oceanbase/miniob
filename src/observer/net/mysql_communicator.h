/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
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

class MysqlCommunicator : public Communicator {
public: 
  virtual RC init(int fd, Session *session, const std::string &addr) override;
  virtual RC read_event(SessionEvent *&event) override;
  virtual RC write_result(SessionEvent *event, bool &need_disconnect) override;

private:
  RC send_packet(const BasePacket &packet);
  RC write_state(SessionEvent *event, bool &need_disconnect);
  RC send_column_definition(SqlResult *sql_result, bool &need_disconnect);
  RC send_result_rows(SqlResult *sql_result, bool &need_disconnect);
  RC handle_version_comment(bool &need_disconnect);

private:
  bool authed_ = false;
  uint32_t client_capabilities_flag_ = 0;
  int8_t sequence_id_ = 0;
};
