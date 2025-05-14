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

#include <string>

#include "net/cli_communicator.h"
#include "common/lang/string.h"
#include "common/log/log.h"
#include "event/session_event.h"
#include "net/buffered_writer.h"
#include "session/session.h"
#include "replxx.hxx"

#define MAX_MEM_BUFFER_SIZE 8192
#define PORT_DEFAULT 6789

using namespace common;

static replxx::Replxx rx;
const std::string     REPLXX_HISTORY_FILE = "./.miniob.history";
// const int MAX_HISTORY_SIZE = 500;  // default: 1000

char *my_readline(const char *prompt)
{
  char const *cinput = nullptr;

  try {
    cinput = rx.input(prompt);
  } catch (std::exception const &e) {
    LOG_WARN("replxx input error: %s", e.what());
    return nullptr;
  }

  if (cinput == nullptr) {
    return nullptr;
  }

  bool is_valid_input = false;
  for (auto c = cinput; *c != '\0'; ++c) {
    if (!isspace(*c)) {
      is_valid_input = true;
      break;
    }
  }

  if (is_valid_input) {
    rx.history_add(cinput);
  }

  char *line = strdup(cinput);
  if (line == nullptr) {
    LOG_WARN("Failed to dup input string from replxx");
    return nullptr;
  }

  return line;
}

/* this function config a exit-cmd list, strncasecmp func truncate the command from terminal according to the number,
   'strncasecmp("exit", cmd, 4)' means that obclient read command string from terminal, truncate it to 4 chars from
   the beginning, then compare the result with 'exit', if they match, exit the obclient.
*/
bool is_exit_command(const char *cmd)
{
  return 0 == strncasecmp("exit", cmd, 4) || 0 == strncasecmp("bye", cmd, 3) || 0 == strncasecmp("\\q", cmd, 2) ||
         0 == strncasecmp("interrupted", cmd, 11);
}

char *read_command()
{
  const char *prompt_str = "miniob > ";

  static bool is_first_call = true;
  if (is_first_call) {
    // rx.set_max_history_size(MAX_HISTORY_SIZE);
    // rx.set_unique_history(true);

    rx.history_load(REPLXX_HISTORY_FILE);
    rx.install_window_change_handler();
    is_first_call = false;
  }

  char *input_command = my_readline(prompt_str);

  static time_t previous_history_save_time = 0;
  if (input_command != nullptr && input_command[0] != '\0') {
    if (time(NULL) - previous_history_save_time > 5) {
      rx.history_save(REPLXX_HISTORY_FILE);
      previous_history_save_time = time(NULL);
    }
  }

  return input_command;
}

RC CliCommunicator::init(int fd, unique_ptr<Session> session, const string &addr)
{
  RC rc = PlainCommunicator::init(fd, std::move(session), addr);
  if (OB_FAIL(rc)) {
    LOG_WARN("fail to init communicator", strrc(rc));
    return rc;
  }

  if (fd == STDIN_FILENO) {
    write_fd_ = STDOUT_FILENO;
    delete writer_;
    writer_ = new BufferedWriter(write_fd_);

    const char delimiter = '\n';
    send_message_delimiter_.assign(1, delimiter);

    fd_ = -1;  // 防止被父类析构函数关闭
  } else {
    rc = RC::INVALID_ARGUMENT;
    LOG_WARN("only stdin supported");
  }
  return rc;
}

RC CliCommunicator::read_event(SessionEvent *&event)
{
  event         = nullptr;
  char *command = read_command();
  if (nullptr == command) {
    return RC::SUCCESS;
  }

  if (is_blank(command)) {
    free(command);
    return RC::SUCCESS;
  }

  if (is_exit_command(command)) {
    free(command);
    exit_ = true;
    return RC::SUCCESS;
  }

  event = new SessionEvent(this);
  event->set_query(string(command));
  free(command);
  return RC::SUCCESS;
}

RC CliCommunicator::write_result(SessionEvent *event, bool &need_disconnect)
{
  RC rc = PlainCommunicator::write_result(event, need_disconnect);

  need_disconnect = false;
  return rc;
}

CliCommunicator::~CliCommunicator()
{
  rx.history_save(REPLXX_HISTORY_FILE);
  LOG_INFO("Command history saved to %s", REPLXX_HISTORY_FILE.c_str());
}
