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

#include <new>
#include <stdlib.h>
namespace common {

/* Prototypes */
bool  check_leaks();
void *operator new(size_t size, const char *file, int line);
void *operator new[](size_t size, const char *file, int line);
#ifndef NO_PLACEMENT_DELETE
void operator delete(void *pointer, const char *file, int line);
void operator delete[](void *pointer, const char *file, int line);
#endif                           // NO_PLACEMENT_DELETE
void operator delete[](void *);  // MSVC 6 requires this declaration

/* Macros */
#ifndef DEBUG_NEW_NO_NEW_REDEFINITION
#define new DEBUG_NEW
#define DEBUG_NEW new (__FILE__, __LINE__)
#define debug_new new
#else
#define debug_new new (__FILE__, __LINE__)
#endif  // DEBUG_NEW_NO_NEW_REDEFINITION
#ifdef DEBUG_NEW_EMULATE_MALLOC

#define malloc(s) ((void *)(debug_new char[s]))

#define free(p) delete[](char *)(p)

#endif  // DEBUG_NEW_EMULATE_MALLOC

/* Control flags */
extern bool new_verbose_flag;    // default to false: no verbose information
extern bool new_autocheck_flag;  // default to true: call check_leaks() on exit

}  // namespace common
