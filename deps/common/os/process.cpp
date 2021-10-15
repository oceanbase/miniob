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
// Created by Longda on 2010
//

#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include <string>

#include "common/io/io.h"
#include "common/log/log.h"
#include "common/os/path.h"
#include "common/os/process.h"
namespace common {

#ifdef __MACH__
#include <libgen.h>
#endif

#define MAX_ERR_OUTPUT 10000000 // 10M
#define MAX_STD_OUTPUT 10000000 // 10M

#define RWRR (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

std::string get_process_name(const char *prog_name) {
  std::string process_name;

  int buf_len = strlen(prog_name);

  assert(buf_len);

  char *buf = new char[buf_len + 1];
  if (buf == NULL) {
    std::cerr << "Failed to alloc memory for program name."
              << SYS_OUTPUT_FILE_POS << SYS_OUTPUT_ERROR << std::endl;
    return "";
  }
  memset(buf, 0, buf_len + 1);
  strncpy(buf, prog_name, buf_len);

  process_name = basename(buf);

  delete[] buf;
  return process_name;
}

// Background the process by detaching it from the console and redirecting
// std in, out, and err to /dev/null
int daemonize_service(bool close_std_streams) {
  int nochdir = 1;
  int noclose = close_std_streams ? 0 : 1;
  int rc = daemon(nochdir, noclose);
  // Here after the fork; the parent is dead and setsid() is called
  if (rc != 0) {
    std::cerr << "Error: unable to daemonize: " << strerror(errno) << "\n";
  }
  return rc;
}

int daemonize_service(const char *std_out_file, const char *std_err_file) {
  int rc = daemonize_service(false);

  if (rc != 0) {
    std::cerr << "Error: \n";
    return rc;
  }

  sys_log_redirect(std_out_file, std_err_file);

  return 0;
}

void sys_log_redirect(const char *std_out_file, const char *std_err_file) {
  int rc = 0;

  // Redirect stdin to /dev/null
  int nullfd = open("/dev/null", O_RDONLY);
  if (nullfd >= 0) {
    dup2(nullfd, STDIN_FILENO);
    close(nullfd);
  }

  // Get timestamp.
  struct timeval tv;
  rc = gettimeofday(&tv, NULL);
  if (rc != 0) {
    std::cerr << "Fail to get current time" << std::endl;
    tv.tv_sec = 0;
  }

  int std_err_flag, std_out_flag;
  // Always use append-write. And if not exist, create it.
  std_err_flag = std_out_flag = O_CREAT | O_APPEND | O_WRONLY;

  std::string err_file = getAboslutPath(std_err_file);

  // CWE367: A check occurs on a file's attributes before the file is 
  // used in a privileged operation, but things may have changed
  // Redirect stderr to std_err_file
  // struct stat st;
  // rc = stat(err_file.c_str(), &st);
  // if (rc != 0 || st.st_size > MAX_ERR_OUTPUT) {
  //   // file may not exist or oversize
  //   std_err_flag |= O_TRUNC; // Remove old content if any.
  // }

  std_err_flag |= O_TRUNC; // Remove old content if any.

  int errfd = open(err_file.c_str(), std_err_flag, RWRR);
  if (errfd >= 0) {
    dup2(errfd, STDERR_FILENO);
    close(errfd);
  }
  setvbuf(stderr, NULL, _IONBF, 0); // Make sure stderr is not buffering
  std::cerr << "Process " << getpid() << " built error output at " << tv.tv_sec
            << std::endl;

  std::string outFile = getAboslutPath(std_out_file);

  // Redirect stdout to outFile.c_str()
  // rc = stat(outFile.c_str(), &st);
  // if (rc != 0 || st.st_size > MAX_STD_OUTPUT) {
  //   // file may not exist or oversize
  //   std_out_flag |= O_TRUNC; // Remove old content if any.
  // }

  std_out_flag |= O_TRUNC; // Remove old content if any.
  int outfd = open(outFile.c_str(), std_out_flag, RWRR);
  if (outfd >= 0) {
    dup2(outfd, STDOUT_FILENO);
    close(outfd);
  }
  setvbuf(stdout, NULL, _IONBF, 0); // Make sure stdout not buffering
  std::cout << "Process " << getpid() << " built standard output at "
            << tv.tv_sec << std::endl;

  return;
}

} //namespace common