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
// Created by Longda on 2021
//

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "common/defs.h"
#include "common/lang/string.h"
#include "common/linereader/line_reader.h"
#include "common/log/log.h"

#define MAX_MEM_BUFFER_SIZE 8192
#define PORT_DEFAULT 6789

using namespace std;
using namespace common;

const std::string LINE_HISTORY_FILE = "./.obclient.history";

int init_unix_sock(const char *unix_sock_path)
{
  int sockfd = socket(PF_UNIX, SOCK_STREAM, 0);
  if (sockfd < 0) {
    fprintf(stderr, "failed to create unix socket. %s", strerror(errno));
    return -1;
  }

  struct sockaddr_un sockaddr;
  memset(&sockaddr, 0, sizeof(sockaddr));
  sockaddr.sun_family = PF_UNIX;
  snprintf(sockaddr.sun_path, sizeof(sockaddr.sun_path), "%s", unix_sock_path);

  if (connect(sockfd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0) {
    fprintf(stderr, "failed to connect to server. unix socket path '%s'. error %s", sockaddr.sun_path, strerror(errno));
    close(sockfd);
    return -1;
  }
  return sockfd;
}

int init_tcp_sock(const char *server_host, int server_port)
{
  struct hostent    *host;
  struct sockaddr_in serv_addr;

  if ((host = gethostbyname(server_host)) == NULL) {
    fprintf(stderr, "gethostbyname failed. errmsg=%d:%s\n", errno, strerror(errno));
    return -1;
  }

  int sockfd;
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    fprintf(stderr, "create socket error. errmsg=%d:%s\n", errno, strerror(errno));
    return -1;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port   = htons(server_port);
  serv_addr.sin_addr   = *((struct in_addr *)host->h_addr);
  bzero(&(serv_addr.sin_zero), 8);

  if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) == -1) {
    fprintf(stderr, "Failed to connect. errmsg=%d:%s\n", errno, strerror(errno));
    close(sockfd);
    return -1;
  }
  return sockfd;
}

const char *startup_tips = R"(
Welcome to the OceanBase database implementation course.

Copyright (c) 2021 OceanBase and/or its affiliates.

Learn more about OceanBase at https://github.com/oceanbase/oceanbase
Learn more about MiniOB at https://github.com/oceanbase/miniob

)";

int main(int argc, char *argv[])
{
  printf("%s", startup_tips);

  const char  *unix_socket_path = nullptr;
  const char  *server_host      = "127.0.0.1";
  int          server_port      = PORT_DEFAULT;
  int          opt;
  extern char *optarg;
  while ((opt = getopt(argc, argv, "s:h:p:")) > 0) {
    switch (opt) {
      case 's': unix_socket_path = optarg; break;
      case 'p': server_port = atoi(optarg); break;
      case 'h': server_host = optarg; break;
    }
  }

  const char *prompt_str = "miniob > ";

  int sockfd, send_bytes;

  if (unix_socket_path != nullptr) {
    sockfd = init_unix_sock(unix_socket_path);
  } else {
    sockfd = init_tcp_sock(server_host, server_port);
  }
  if (sockfd < 0) {
    return 1;
  }

  char send_buf[MAX_MEM_BUFFER_SIZE];

  std::string input_command = "";
  MiniobLineReader::instance().init(LINE_HISTORY_FILE);

  while (true) {
    input_command = MiniobLineReader::instance().my_readline(prompt_str);

    if (input_command.empty() || common::is_blank(input_command.c_str())) {
      continue;
    }

    if (MiniobLineReader::instance().is_exit_command(input_command)) {
      break;
    }

    if ((send_bytes = write(sockfd, input_command.c_str(), input_command.length() + 1)) == -1) {  // TODO writen
      fprintf(stderr, "send error: %d:%s \n", errno, strerror(errno));
      exit(1);
    }

    memset(send_buf, 0, sizeof(send_buf));

    int len = 0;
    while ((len = recv(sockfd, send_buf, MAX_MEM_BUFFER_SIZE, 0)) > 0) {
      bool msg_end = false;
      for (int i = 0; i < len; i++) {
        if (0 == send_buf[i]) {
          msg_end = true;
          break;
        }
        printf("%c", send_buf[i]);
      }
      if (msg_end) {
        break;
      }
      memset(send_buf, 0, MAX_MEM_BUFFER_SIZE);
    }

    if (len < 0) {
      fprintf(stderr, "Connection was broken: %s\n", strerror(errno));
      break;
    }
    if (0 == len) {
      printf("Connection has been closed\n");
      break;
    }
  }

  close(sockfd);

  return 0;
}
