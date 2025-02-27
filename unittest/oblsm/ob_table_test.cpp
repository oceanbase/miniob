/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "gtest/gtest.h"

#include "common/lang/filesystem.h"
#include "oblsm/util/ob_comparator.h"
#include "oblsm/table/ob_sstable_builder.h"
#include "oblsm/table/ob_sstable.h"

using namespace oceanbase;

TEST(table_test, DISABLED_table_test_basic)
{
  ObDefaultComparator comparator;
  shared_ptr<ObMemTable> table = make_shared<ObMemTable>();
  uint64_t seq = 0;
  size_t count = 5;
  for (size_t i = 0; i < count; i++) {
    string key(to_string(i));
    table->put(seq++, key, key);
  }
  ObSSTableBuilder tb(&comparator, nullptr);
  ASSERT_EQ(tb.build(table, "test.sst", 0), RC::SUCCESS);
  shared_ptr<ObSSTable> sst = tb.get_built_table();
  ObLsmIterator* sst_iter = sst->new_iterator();
  sst_iter->seek_to_first();
  while(sst_iter->valid()) {
    cout << sst_iter->key() << " " << sst_iter->value() << endl;
    sst_iter->next();
  }
  delete sst_iter;

}


int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}