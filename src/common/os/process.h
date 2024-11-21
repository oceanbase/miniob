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
// Created by Longda on 2010
//

#pragma once

#include "common/lang/string.h"

namespace common {

//! Get process Name
/**
 * @param[in]   prog_full_name  process full name with full path
 * @return      process_name_   process name without directory path
 */
string get_process_name(const char *prog_full_name);
//! Runs the service as a daemon
/**
 * Backgrounds the calling service as a system daemon by detaching it from
 * the controlling terminal, closes stdin, and reopens stdout and stderr
 * to the files specified in the input parmaters. "/dev/null" is accepted as
 * a valid input, which will be equivalent to closing the respective stream.
 * Keeping the streams open but reopening them allows the streams of the
 * controling terminal to be closed, thus making it possible for the terminal
 * to exit normally while the service is backgrounded. The same file
 * could be used for reopening both stderr and stdout streams.
 * Creates a new session and sets the service process as the group parent.
 *
 * @param[in]   std_out_file  file to redirect stdout to (could be /dev/null)
 * @param[in]   std_err_file  file to redirect stderr to (could be /dev/null)
 * @return  0 if success, error code otherwise
 */
int daemonize_service(const char *std_out_file, const char *std_err_file);

void sys_log_redirect(const char *std_out_file, const char *std_err_file);

}  // namespace common
