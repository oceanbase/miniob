/* Copyright (c) 2021-2022 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by huhaosheng.hhs on 2022
//

#include <string.h>

#include "common/log/log.h"
#include "storage/clog/clog.h"
#include "gtest/gtest.h"

using namespace common;

TEST(test_clog, test_clog)
{
  const char *path = ".";
  const char *clog_file = "./clog";
  remove(clog_file);
  

  CLogManager log_mgr;
  RC rc = log_mgr.init(path);
  ASSERT_EQ(rc, RC::SUCCESS);
  // TODO test

  /*
  // record 已经被删掉了，不能再访问
  int i = 0;
  for (auto iter = log_mtr_mgr->log_redo_list.begin(); iter != log_mtr_mgr->log_redo_list.end();
       iter++) {
    CLogRecord *tmp = *iter;
    ASSERT_EQ(1, log_rec[i]->cmp_eq(tmp));
    delete tmp;
    i++;
  }
  */
}

int main(int argc, char **argv)
{
  // 分析gtest程序的命令行参数
  testing::InitGoogleTest(&argc, argv);

  LoggerFactory::init_default("clog_test.log", LOG_LEVEL_TRACE);
  
  // 调用RUN_ALL_TESTS()运行所有测试用例
  // main函数返回RUN_ALL_TESTS()的运行结果
  return RUN_ALL_TESTS();
}
