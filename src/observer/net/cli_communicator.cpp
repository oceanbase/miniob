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

#include "net/cli_communicator.h"
#include "net/buffered_writer.h"
#include "common/log/log.h"
#include "common/lang/string.h"
#include "event/session_event.h"

#ifdef USE_READLINE
#include "readline/readline.h"
#include "readline/history.h"
#endif

#define MAX_MEM_BUFFER_SIZE 8192
#define PORT_DEFAULT 6789

using namespace common;

#ifdef USE_READLINE
const std::string HISTORY_FILE = std::string(getenv("HOME")) + "/.miniob.history";
time_t last_history_write_time = 0;

char *my_readline(const char *prompt) 
{
  int size = history_length;
  if (size == 0) {
    read_history(HISTORY_FILE.c_str());

    FILE *fp = fopen(HISTORY_FILE.c_str(), "a");
    if (fp != nullptr) {
      fclose(fp);
    }
  }

  char *line = readline(prompt);
  if (line != nullptr && line[0] != 0) {
    add_history(line);
    if (time(NULL) - last_history_write_time > 5) {
      write_history(HISTORY_FILE.c_str());
    }
    // append_history doesn't work on some readlines
    // append_history(1, HISTORY_FILE.c_str());
  }
  return line;
}
#else // USE_READLINE
char *my_readline(const char *prompt)
{
  char *buffer = (char *)malloc(MAX_MEM_BUFFER_SIZE);
  if (nullptr == buffer) {
    LOG_WARN("failed to alloc line buffer");
    return nullptr;
  }
  fprintf(stdout, "%s", prompt);
  char *s = fgets(buffer, MAX_MEM_BUFFER_SIZE, stdin);
  if (nullptr == s) {
    free(buffer);
    if (ferror(stdin) || feof(stdin)) {
      LOG_WARN("failed to read line: %s", strerror(errno));
    }
    return nullptr;
  }
  return buffer;
}
#endif // USE_READLINE

/* this function config a exit-cmd list, strncasecmp func truncate the command from terminal according to the number,
   'strncasecmp("exit", cmd, 4)' means that obclient read command string from terminal, truncate it to 4 chars from 
   the beginning, then compare the result with 'exit', if they match, exit the obclient.
*/
bool is_exit_command(const char *cmd) {
  return 0 == strncasecmp("exit", cmd, 4) ||
         0 == strncasecmp("bye", cmd, 3) ||
         0 == strncasecmp("\\q", cmd, 2) ;
}

char *read_command()
{
  const char *prompt_str = "miniob > ";
  char *input_command = nullptr;
  for (input_command = my_readline(prompt_str); 
        common::is_blank(input_command); 
        input_command = my_readline(prompt_str)) {
    free(input_command);
    input_command = nullptr;
  }
  return input_command;
}

RC CliCommunicator::init(int fd, Session *session, const std::string &addr)
{
  RC rc = PlainCommunicator::init(fd, session, addr);
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

    fd_ = -1; // 防止被父类析构函数关闭
  } else {
    rc = RC::INVALID_ARGUMENT;
    LOG_WARN("only stdin supported");
  }
  return rc;
}

RC CliCommunicator::read_event(SessionEvent *&event)
{
  event = nullptr;
  char *command = read_command();

  if (is_exit_command(command)) {
    free(command);
    event = nullptr;
    return RC::SUCCESS;
  }

  event = new SessionEvent(this);
  event->set_query(std::string(command));
  free(command);
  return RC::SUCCESS;
}

RC CliCommunicator::write_result(SessionEvent *event, bool &need_disconnect)
{
  RC rc = PlainCommunicator::write_result(event, need_disconnect);
  need_disconnect = false;
  return rc;
}
