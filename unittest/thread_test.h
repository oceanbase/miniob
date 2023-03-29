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

#ifndef CTESTTHREAD_H_
#define CTESTTHREAD_H_

#include "common/lang/mutex.h"

/*
 *
 */
class ThreadTest {
public:
  ThreadTest();
  virtual ~ThreadTest();

  int create();

  int startTestCond();
  int startTestDeadLock();

  static void *testCond(void *param);
  static void *testDeadLock(void *param);

private:
  int param[10];

  pthread_mutex_t mutex;
  pthread_cond_t cond;

  pthread_mutex_t dead_mutex1;
  pthread_mutex_t dead_mutex2;
};

#endif /* CTESTTHREAD_H_ */
