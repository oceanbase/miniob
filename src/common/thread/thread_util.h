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
// Created by Wangyunlai on 2023/01/15.
//

#pragma once

namespace common {

/**
 * @brief 设置当前线程的名字
 * @details 设置当前线程的名字可以帮助调试多线程程序，比如在gdb或者 top -H命令可以看到线程名字。
 * pthread_setname_np在Linux和Mac上实现不同。Linux上可以指定线程号设置名称，但是Mac上不行。
 * @param name 线程的名字。按照linux手册中描述，包括\0在内，不要超过16个字符
 * @return int 设置成功返回0
 */
int thread_set_name(const char *name);

}  // namespace common