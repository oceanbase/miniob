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

#include <signal.h>

namespace common {

//! Default function that blocks signals.
/**
 * Now it blocks SIGINT, SIGTERM, and SIGUSR1
 */
void block_default_signals(sigset_t *signal_set, sigset_t *old_set);
//! Default function that unblocks signals.
/**
 * It unblocks SIGINT, SIGTERM,and SIGUSR1.
 */
void unblock_default_signals(sigset_t *signal_set, sigset_t *old_set);

void *wait_for_signals(sigset_t *signal_set);
void  start_wait_for_signals(sigset_t *signal_set);

// Set signal handling function
/**
 * handler function
 */
typedef void (*sighandler_t)(int);
void set_signal_handler(sighandler_t func);
void set_signal_handler(int sig, sighandler_t func);

}  // namespace common
