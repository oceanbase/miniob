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
// Created by Wangyunlai on 2022/11/17.
//

#include "net/communicator.h"
#include "net/mysql_communicator.h"
#include "sql/expr/tuple.h"
#include "event/session_event.h"
#include "common/lang/mutex.h"
#include "common/io/io.h"
#include "session/session.h"

RC Communicator::init(int fd, Session *session, const std::string &addr)
{
  fd_ = fd;
  session_ = session;
  addr_ = addr;
  return RC::SUCCESS;
}

Communicator::~Communicator()
{
  if (fd_ >= 0) {
    close(fd_);
    fd_ = -1;
  }
  if (session_ != nullptr) {
    delete session_;
    session_ = nullptr;
  }
}

/////////////////////////////////////////////////////////////////////////////////
RC PlainCommunicator::read_event(SessionEvent *&event)
{
  RC rc = RC::SUCCESS;

  event = nullptr;

  int data_len = 0;
  int read_len = 0;

  const int max_packet_size = 8192;
  std::vector<char> buf(max_packet_size);

  // 持续接收消息，直到遇到'\0'。将'\0'遇到的后续数据直接丢弃没有处理，因为目前仅支持一收一发的模式
  while (true) {
    read_len = ::read(fd_, buf.data() + data_len, max_packet_size - data_len);
    if (read_len < 0) {
      if (errno == EAGAIN) {
        continue;
      }
      break;
    }
    if (read_len == 0) {
      break;
    }

    if (read_len + data_len > max_packet_size) {
      data_len += read_len;
      break;
    }

    bool msg_end = false;
    for (int i = 0; i < read_len; i++) {
      if (buf[data_len + i] == 0) {
        data_len += i + 1;
        msg_end = true;
        break;
      }
    }

    if (msg_end) {
      break;
    }

    data_len += read_len;
  }

  if (data_len > max_packet_size) {
    LOG_WARN("The length of sql exceeds the limitation %d", max_packet_size);
    return RC::IOERR;
  }
  if (read_len == 0) {
    LOG_INFO("The peer has been closed %s\n", addr());
    return RC::IOERR;
  } else if (read_len < 0) {
    LOG_ERROR("Failed to read socket of %s, %s\n", addr(), strerror(errno));
    return RC::IOERR;
  }

  LOG_INFO("receive command(size=%d): %s", data_len, buf.data());
  event = new SessionEvent(this);
  event->set_query(std::string(buf.data()));
  return rc;
}

RC PlainCommunicator::write_state(SessionEvent *event, bool &need_disconnect)
{
  SqlResult *sql_result = event->sql_result();
  const int buf_size = 2048;
  char *buf = new char[buf_size];
  const std::string &state_string = sql_result->state_string();
  if (state_string.empty()) {
    const char *result = RC::SUCCESS == sql_result->return_code() ? "SUCCESS" : "FAILURE";
    snprintf(buf, buf_size, "%s\n", result);
  } else {
    snprintf(buf, buf_size, "%s > %s\n", strrc(sql_result->return_code()), state_string.c_str());
  }

  int ret = common::writen(fd_, buf, strlen(buf) + 1);
  if (ret != 0) {
    LOG_WARN("failed to send data to client. err=%s", strerror(errno));
    need_disconnect = true;
    delete[] buf;
    return RC::IOERR;
  }

  need_disconnect = false;
  delete[] buf;
  return RC::SUCCESS;
}

RC PlainCommunicator::write_result(SessionEvent *event, bool &need_disconnect)
{
  need_disconnect = true;

  const char message_terminate = '\0';

  SqlResult *sql_result = event->sql_result();
  if (nullptr == sql_result) {

    const char *response = event->get_response();
    int len = event->get_response_len();

    int ret = common::writen(fd_, response, len);
    if (ret < 0) {
      LOG_ERROR("Failed to send data back to client. ret=%d, error=%s", ret, strerror(errno));

      return RC::IOERR;
    }
    ret = common::writen(fd_, &message_terminate, sizeof(message_terminate));
    if (ret < 0) {
      LOG_ERROR("Failed to send data back to client. ret=%d, error=%s", ret, strerror(errno));
      return RC::IOERR;
    }

    need_disconnect = false;
    return RC::SUCCESS;
  }

  if (RC::SUCCESS != sql_result->return_code() || !sql_result->has_operator()) {
    return write_state(event, need_disconnect);
  }

  RC rc = sql_result->open();
  if (rc != RC::SUCCESS) {
    sql_result->set_return_code(rc);
    return write_state(event, need_disconnect);
  }

  const TupleSchema &schema = sql_result->tuple_schema();
  const int cell_num = schema.cell_num();

  for (int i = 0; i < cell_num; i++) {
    const TupleCellSpec &spec = schema.cell_at(i);
    const char *alias = spec.alias();
    if (nullptr != alias || alias[0] != 0) {
      if (0 != i) {
        const char *delim = " | ";
        int ret = common::writen(fd_, delim, strlen(delim));
        if (ret != 0) {
          LOG_WARN("failed to send data to client. err=%s", strerror(errno));
          return RC::IOERR;
        }
      }

      int len = strlen(alias);
      int ret = common::writen(fd_, alias, len);
      if (ret != 0) {
        LOG_WARN("failed to send data to client. err=%s", strerror(errno));
        return RC::IOERR;
      }
    }
  }

  if (cell_num > 0) {
    char newline = '\n';
    int ret = common::writen(fd_, &newline, 1);
    if (ret != 0) {
      LOG_WARN("failed to send data to client. err=%s", strerror(errno));
      return RC::IOERR;
    }
  }

  rc = RC::SUCCESS;
  Tuple *tuple = nullptr;
  while (RC::SUCCESS == (rc = sql_result->next_tuple(tuple))) {
    assert(tuple != nullptr);

    int cell_num = tuple->cell_num();
    for (int i = 0; i < cell_num; i++) {
      if (i != 0) {
        const char *delim = " | ";
        int ret = common::writen(fd_, delim, strlen(delim));
        if (ret != 0) {
          LOG_WARN("failed to send data to client. err=%s", strerror(errno));
          return RC::IOERR;
        }
      }

      TupleCell cell;
      rc = tuple->cell_at(i, cell);
      if (rc != RC::SUCCESS) {
        return rc;
      }

      std::stringstream ss;
      cell.to_string(ss);
      std::string cell_str = ss.str();
      int ret = common::writen(fd_, cell_str.data(), cell_str.size());
      if (ret != RC::SUCCESS) {
        LOG_WARN("failed to send data to client. err=%s", strerror(errno));
        return RC::IOERR;
      }
    }

    char newline = '\n';
    int ret = common::writen(fd_, &newline, 1);
    if (ret != 0) {
      LOG_WARN("failed to send data to client. err=%s", strerror(errno));
      return RC::IOERR;
    }
  }

  if (rc == RC::RECORD_EOF) {
    rc = RC::SUCCESS;
  }

  if (cell_num == 0) {
    // 除了select之外，其它的消息通常不会通过operator来返回结果，表头和行数据都是空的
    // 这里针对这种情况做特殊处理，当表头和行数据都是空的时候，就返回处理的结果
    // 可能是insert/delete等操作，不直接返回给客户端数据，这里把处理结果返回给客户端
    sql_result->set_return_code(rc);
    return write_state(event, need_disconnect);
  } else {

    int ret = common::writen(fd_, &message_terminate, sizeof(message_terminate));
    if (ret < 0) {
      LOG_ERROR("Failed to send data back to client. ret=%d, error=%s", ret, strerror(errno));
      return RC::IOERR;
    }

    need_disconnect = false;
  }
  return rc;
}

/////////////////////////////////////////////////////////////////////////////////

Communicator *CommunicatorFactory::create(CommunicateProtocol protocol)
{
  switch (protocol) {
    case CommunicateProtocol::PLAIN: {
      return new PlainCommunicator;
    } break;
    case CommunicateProtocol::MYSQL: {
      return new MysqlCommunicator;
    } break;
    default: {
      return nullptr;
    }
  }
}
