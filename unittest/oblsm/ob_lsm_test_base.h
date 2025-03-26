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
#include "common/lang/thread.h"
#include "common/lang/utility.h"
#include "oblsm/include/ob_lsm.h"
#include "oblsm/ob_lsm_define.h"

using namespace oceanbase;

class KeyValueGenerator
{
public:
  static vector<pair<string, string>> generate_data(size_t count)
  {
    vector<pair<string, string>> data;
    for (size_t i = 0; i < count; ++i) {
      data.emplace_back("key" + to_string(i), "value" + to_string(i));
    }
    return data;
  }
};

class ObLsmTestBase : public ::testing::TestWithParam<size_t>
{
protected:
  ObLsm       *db;
  ObLsmOptions options;
  string       path;

  void SetUp() override
  {
    path = "./testdb";
    set_up_options();
    filesystem::remove_all(path);
    filesystem::create_directory(path);
    ASSERT_EQ(ObLsm::open(options, path, &db), RC::SUCCESS);
    ASSERT_NE(db, nullptr);
  }

  void set_up_options()
  {
    options.memtable_size         = 8 * 1024;
    options.table_size            = 16 * 1024;
    options.default_levels        = 7;
    options.default_l1_level_size = 128 * 1024;
    options.default_level_ratio   = 10;
    options.default_l0_file_num   = 3;
    options.default_run_num       = 7;
    options.type                  = CompactionType::LEVELED;
  }

  void TearDown() override { delete db; }
};