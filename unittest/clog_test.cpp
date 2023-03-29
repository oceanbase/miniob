/* Copyright (c) 2021-2022 Xie Meiyi(xiemeiyi@hust.edu.cn),
Huazhong University of Science and Technology
and OceanBase and/or its affiliates. All rights reserved.
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

#include "storage/clog/clog.h"
#include "gtest/gtest.h"

Record *gen_ins_record(int32_t page_num, int32_t slot_num, int data_len)
{
  Record *rec = new Record();
  char *data = new char[data_len];
  rec->set_rid(page_num, slot_num);
  memset(data, data_len, data_len);
  rec->set_data(data);
  return rec;
}

Record *gen_del_record(int32_t page_num, int32_t slot_num)
{
  Record *rec = new Record();
  rec->set_rid(page_num, slot_num);
  return rec;
}

TEST(test_clog, test_clog)
{
  CLogManager *log_mgr = new CLogManager("/home/huhaosheng.hhs/Alibaba/miniob");

  CLogRecord *log_rec[6];
  CLogRecord *log_mtr_rec = nullptr;
  Record *rec = nullptr;
  //
  log_mgr->clog_gen_record(REDO_MTR_BEGIN, 1, log_mtr_rec);
  log_mgr->clog_append_record(log_mtr_rec);  // NOTE: 需要保留log_rec
  delete log_mtr_rec;

  rec = gen_ins_record(1, 1, 100);
  log_mgr->clog_gen_record(REDO_INSERT, 1, log_rec[0], "table1", 100, rec);
  log_mgr->clog_append_record(log_rec[0]);
  delete[] rec->data();
  delete rec;

  rec = gen_ins_record(1, 1, 120);
  log_mgr->clog_gen_record(REDO_INSERT, 1, log_rec[1], "table2", 120, rec);
  log_mgr->clog_append_record(log_rec[1]);
  delete[] rec->data();
  delete rec;

  log_mgr->clog_gen_record(REDO_MTR_BEGIN, 2, log_mtr_rec);
  log_mgr->clog_append_record(log_mtr_rec);
  delete log_mtr_rec;

  rec = gen_ins_record(1, 1, 200);
  log_mgr->clog_gen_record(REDO_INSERT, 1, log_rec[2], "table3", 200, rec);
  log_mgr->clog_append_record(log_rec[2]);
  delete[] rec->data();
  delete rec;

  rec = gen_ins_record(1, 2, 120);
  log_mgr->clog_gen_record(REDO_INSERT, 2, log_rec[3], "table2", 120, rec);
  log_mgr->clog_append_record(log_rec[3]);
  delete[] rec->data();
  delete rec;

  rec = gen_ins_record(1, 2, 100);
  log_mgr->clog_gen_record(REDO_INSERT, 2, log_rec[4], "table1", 100, rec);
  log_mgr->clog_append_record(log_rec[4]);
  delete[] rec->data();
  delete rec;

  log_mgr->clog_gen_record(REDO_MTR_COMMIT, 2, log_mtr_rec);
  log_mgr->clog_append_record(log_mtr_rec);
  delete log_mtr_rec;

  rec = gen_del_record(1, 1);
  log_mgr->clog_gen_record(REDO_DELETE, 1, log_rec[5], "table1", 0, rec);
  log_mgr->clog_append_record(log_rec[5]);
  delete rec;

  log_mgr->clog_gen_record(REDO_MTR_COMMIT, 1, log_mtr_rec);
  log_mgr->clog_append_record(log_mtr_rec);
  delete log_mtr_rec;

  log_mgr->recover();

  CLogMTRManager *log_mtr_mgr = log_mgr->get_mtr_manager();

  ASSERT_EQ(true, log_mtr_mgr->trx_commited[1]);
  ASSERT_EQ(true, log_mtr_mgr->trx_commited[2]);

  ASSERT_EQ(6, log_mtr_mgr->log_redo_list.size());

  int i = 0;
  for (auto iter = log_mtr_mgr->log_redo_list.begin(); iter != log_mtr_mgr->log_redo_list.end();
       iter++) {
    CLogRecord *tmp = *iter;
    ASSERT_EQ(1, log_rec[i]->cmp_eq(tmp));
    delete tmp;
    i++;
  }

  for (i = 0; i < 6; i++) {
    delete log_rec[i];
  }
}

int main(int argc, char **argv)
{
  // 分析gtest程序的命令行参数
  testing::InitGoogleTest(&argc, argv);

  // 调用RUN_ALL_TESTS()运行所有测试用例
  // main函数返回RUN_ALL_TESTS()的运行结果
  return RUN_ALL_TESTS();
}