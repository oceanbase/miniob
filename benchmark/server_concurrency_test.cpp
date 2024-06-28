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
// Created by Wangyunlai on 2023/04/28
//

#if 0
#include <inttypes.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <benchmark/benchmark.h>
#include <random>

#include "common/lang/stdexcept.h"
#include "common/log/log.h"
#include "rc.h"

using namespace std;
using namespace common;
using namespace benchmark;

class Client
{
public:
  Client() = default;
  virtual ~Client();

  RC init(string host, int port);
  RC init(string unix_socket);

  RC close();

  RC send_sql(const char *sql);
  RC receive_result(ostream &result_stream);
  RC execute(const char *sql, ostream &result_stream);

private:
  string server_addr_;
  int    socket_ = -1;
};

class BenchmarkBase : public Fixture
{
public:
  BenchmarkBase()
  {}

  virtual ~BenchmarkBase()
  {}

  string Name() const override = 0;
  void SetUp(const State &state) override
  {
    
  }

  void TearDown(const State &state) override
  {
    
  }
};

Client::~Client()
{
  this->close();
}

RC Client::init(string host, int port)
{
  struct hostent *host;
  struct sockaddr_in serv_addr;

  if ((host = gethostbyname(host.c_str())) == NULL) {
    LOG_WARN("failed to gethostbyname. rc=%s", strerror(errno));
    return RC::IOERR_READ;
  }

  int sockfd = -1;
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    LOG_WARN("failed to create socket. rc=%s", strerror(errno));
    return RC::IOERR_OPEN;
  }

  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons((uint16_t)port);
  serv_addr.sin_addr = *((struct in_addr *)host->h_addr);

  if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) == -1) {
    LOG_WARN("failed to connect to server. rc=%s", strerror(errno));
    return RC::IOERROR;
  }

  socket_ = sockfd;
  server_addr_ = string("tcp://") + host + string(":") + to_string(port);
  LOG_INFO("connect to server sucess");
  return RC::SUCCESS;
}

RC Client::init(string unix_socket)
{
  int socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (socket_fd < 0) {
    LOG_WARN("failed to create socket. rc=%s", strerror(errno));
    return RC::IOERROR;
  }

  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strcpy(addr.sun_path, unix_socket.c_str());

  int ret = connect(socket_fd, (struct sockaddr*)&addr, sizeof(addr));
  if (ret == -1) {
    LOG_WARN("failed to connect to server. rc=%s", strerror(errno));
    return RC::IOERROR;
  }

  socket_ = socket_fd;
  server_addr_ = string("unix://") + unix_socket;
  LOG_INFO("connect to unix socket success");
  return RC::SUCCESS;
}

RC Client::close()
{
  if (socket_ >= 0) {
    LOG_INFO("close connection. server addr: %s", server_addr_.c_str());
    ::close(socket_);
    socket_ = -1;
  }
  return RC::SUCESS;
}

RC Client::send_sql(const char *sql)
{
  int ret = writen(socket_, sql, strlen(sql) + 1);
  if (ret != 0) {
    LOG_WARN("failed to send sql to server. server=%s, rc=%s", server_addr_.c_str(), strerror(ret));
    return RC::IOERROR;
  }
  return RC::SUCCESS;
}

RC Client::receive_result(ostream &result_stream)
{
  char tmp_buf[256];

  // 持续接收消息，直到遇到'\0'。将'\0'遇到的后续数据直接丢弃没有处理，因为目前仅支持一收一发的模式
  int read_len = 0;
  while (true) {
    read_len = ::read(socket_, tmp_buf, sizeof(tmp_buf));
    if (read_len < 0) {
      if (errno == EAGAIN) {
        continue;
      }
      break;
    }
    if (read_len == 0) {
      break;
    }

    result_stream.write(tmp_buf, read_len);

    bool msg_end = false;
    for (int i = 0; i < read_len; i++) {
      if (tmp_buf[i] == 0) {
        msg_end = true;
        break;
      }
    }

    if (msg_end) {
      break;
    }
  }
}

RC Client::execute(const char *sql, ostream &result_stream)
{
  RC rc = this->send_sql(sql);
  if (rc != RC::SUCCESS) {
    return rc;
  }

  return receive_result(result_stream);
}

#endif

int main(void) { return 0; }