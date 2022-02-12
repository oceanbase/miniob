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
#include <unistd.h>
#include <termios.h>

#include "common/defs.h"
#include "common/lang/string.h"

#define MAX_MEM_BUFFER_SIZE 8192
#define PORT_DEFAULT 6789

using namespace common;

bool is_exit_command(const char *cmd)
{
  return 0 == strncasecmp("exit", cmd, 4) || 0 == strncasecmp("bye", cmd, 3);
}

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
  struct hostent *host;
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
  serv_addr.sin_port = htons(server_port);
  serv_addr.sin_addr = *((struct in_addr *)host->h_addr);
  bzero(&(serv_addr.sin_zero), 8);

  if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) == -1) {
    fprintf(stderr, "Failed to connect. errmsg=%d:%s\n", errno, strerror(errno));
    close(sockfd);
    return -1;
  }
  return sockfd;
}

// 这里的代码本来是为了处理控制台不能接收超长字符串的问题
// 但是设置控制台模式为非 ICANON 后，不能再正常的处理 backspace
// 所以暂时不调用这个函数
// 需要测试超长字符串场景的同学，可以通过文本重定向的方式测试
int set_terminal_noncanonical()
{
  int fd = STDIN_FILENO;
  struct termios old_termios;
  int ret = tcgetattr(fd, &old_termios);
  if (ret < 0) {
    printf("Failed to get tc attr. error=%s\n", strerror(errno));
    return -1;
  }

  struct termios new_attr = old_termios;
  new_attr.c_lflag &= ~ICANON;
  new_attr.c_cc[VERASE] = '\b';
  ret = tcsetattr(fd, TCSANOW, &new_attr);
  if (ret < 0) {
    printf("Failed to set tc attr. error=%s\n", strerror(errno));
    return -1;
  }
  return 0;
}

int main(int argc, char *argv[])
{
  int ret = 0;  // set_terminal_noncanonical();
  if (ret < 0) {
    printf("Warning: failed to set terminal non canonical. Long command may be handled incorrect\n");
  }

  const char *unix_socket_path = nullptr;
  const char *server_host = "127.0.0.1";
  int server_port = PORT_DEFAULT;
  int opt;
  extern char *optarg;
  while ((opt = getopt(argc, argv, "s:h:p:")) > 0) {
    switch (opt) {
      case 's':
        unix_socket_path = optarg;
        break;
      case 'p':
        server_port = atoi(optarg);
        break;
      case 'h':
        server_host = optarg;
        break;
    }
  }

  const char *prompt_str = "miniob > ";

  int sockfd, send_bytes;
  // char send[MAXLINE];

  if (unix_socket_path != nullptr) {
    sockfd = init_unix_sock(unix_socket_path);
  } else {
    sockfd = init_tcp_sock(server_host, server_port);
  }
  if (sockfd < 0) {
    return 1;
  }

  char send_buf[MAX_MEM_BUFFER_SIZE];
  // char buf[MAXDATASIZE];

  fputs(prompt_str, stdout);
  while (fgets(send_buf, MAX_MEM_BUFFER_SIZE, stdin) != NULL) {
    if (common::is_blank(send_buf)) {
      fputs(prompt_str, stdout);
      continue;
    }

    if (is_exit_command(send_buf)) {
      break;
    }

    if ((send_bytes = write(sockfd, send_buf, strlen(send_buf) + 1)) == -1) {
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
    fputs(prompt_str, stdout);
  }
  close(sockfd);

  return 0;
}
