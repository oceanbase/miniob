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


#include "log_test.h"


#include "gtest/gtest.h"

#include "common/log/log.h"


using namespace common;

LogTest::LogTest() {
  // Auto-generated constructor stub
}

LogTest::~LogTest() {
  // Auto-generated destructor stub
}

int LogTest::init(const std::string &logFile) {



  LoggerFactory::init_default(logFile);

  g_log->set_rotate_type(LOG_ROTATE_BYSIZE);

  return 0;
}

void *LogTest::log_loop(void *param) {
  int index = *(int *) param;
  int i = 0;
  while (i < 100) {
    i++;
    LOG_INFO("index:%d --> %d", index, i);
  }

  return NULL;
}

void checkRotate() {
  LogTest test;

  test.init();
  ASSERT_EQ(g_log->get_rotate_type(), LOG_ROTATE_BYSIZE);

  int index = 30;
  test.log_loop(&index);
}

TEST(checkRotateTest, CheckRoateTest)
{

}

void testEnableTest() {
  LogTest test;

  test.init();


  ASSERT_EQ(g_log->check_output(LOG_LEVEL_PANIC, __FILE__), true);
  ASSERT_EQ(g_log->check_output(LOG_LEVEL_ERR, __FILE__), true);
  ASSERT_EQ(g_log->check_output(LOG_LEVEL_WARN, __FILE__), true);
  ASSERT_EQ(g_log->check_output(LOG_LEVEL_INFO, __FILE__), true);
  ASSERT_EQ(g_log->check_output(LOG_LEVEL_DEBUG, __FILE__), false);
  ASSERT_EQ(g_log->check_output(LOG_LEVEL_TRACE, __FILE__), false);
  ASSERT_EQ(g_log->check_output(LOG_LEVEL_LAST, __FILE__), false);

  g_log->set_default_module(__FILE__);

  ASSERT_EQ(g_log->check_output(LOG_LEVEL_PANIC, __FILE__), true);
  ASSERT_EQ(g_log->check_output(LOG_LEVEL_ERR, __FILE__), true);
  ASSERT_EQ(g_log->check_output(LOG_LEVEL_WARN, __FILE__), true);
  ASSERT_EQ(g_log->check_output(LOG_LEVEL_INFO, __FILE__), true);
  ASSERT_EQ(g_log->check_output(LOG_LEVEL_DEBUG, __FILE__), true);
  ASSERT_EQ(g_log->check_output(LOG_LEVEL_TRACE, __FILE__), true);
  ASSERT_EQ(g_log->check_output(LOG_LEVEL_LAST, __FILE__), true);
}

TEST(testEnableTest, CheckEnableTest)
{

}

int main(int argc, char **argv) {


  // 分析gtest程序的命令行参数
  testing::InitGoogleTest(&argc, argv);

  // 调用RUN_ALL_TESTS()运行所有测试用例
  // main函数返回RUN_ALL_TESTS()的运行结果
  return RUN_ALL_TESTS();
}
