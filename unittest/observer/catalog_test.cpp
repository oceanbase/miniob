/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "catalog/catalog.h"

#include "gtest/gtest.h"

TEST(CatalogTest, catalog_test)
{
  Catalog& catalog = Catalog::get_instance();
    
  int table_id = 1;
  TableStats initial_stats(100);
  TableStats updated_stats(150);

  catalog.update_table_stats(table_id, initial_stats);
  EXPECT_EQ(catalog.get_table_stats(table_id).row_nums, 100);

  catalog.update_table_stats(table_id, updated_stats);
  EXPECT_EQ(catalog.get_table_stats(table_id).row_nums, 150);

  EXPECT_EQ(catalog.get_table_stats(-1).row_nums, 0);
}

int main(int argc, char **argv)
{

  // 分析gtest程序的命令行参数
  testing::InitGoogleTest(&argc, argv);

  // 调用RUN_ALL_TESTS()运行所有测试用例
  // main函数返回RUN_ALL_TESTS()的运行结果
  return RUN_ALL_TESTS();
}
