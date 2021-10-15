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
// Created by Longda on 2021/4/13.
//

#ifndef __SRC_OBSERVER_NET_SERVER_PARAM_H__
#define __SRC_OBSERVER_NET_SERVER_PARAM_H__

#include <string>

class ServerParam {
public:
  ServerParam();

  ServerParam(const ServerParam &other) = default;
  ~ServerParam() = default;

public:
  // accpet client's address, default is INADDR_ANY, means accept every address
  long listen_addr;

  int max_connection_num;
  // server listing port
  int port;

  std::string unix_socket_path;

  // 如果使用标准输入输出作为通信条件，就不再监听端口
  bool use_unix_socket = false;
};

#endif //__SRC_OBSERVER_NET_SERVER_PARAM_H__
