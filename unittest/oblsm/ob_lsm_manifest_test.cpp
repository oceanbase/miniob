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
// Created by Ping Xu(haibarapink@gmail.com)
//
#include "gtest/gtest.h"

#include "common/lang/filesystem.h"
#include "oblsm/include/ob_lsm.h"
#include "oblsm/include/ob_lsm_options.h"
#include "oblsm/ob_lsm_define.h"
#include "oblsm/ob_manifest.h"

using namespace oceanbase;

TEST(oblsm_manifest_test, DISABLED_record_serialization_and_deserialization)
{
  // Compaction
  ObManifestCompaction compaction;
  compaction.compaction_type     = CompactionType::LEVELED;
  compaction.sstable_sequence_id = 100;
  compaction.seq_id              = 200;
  compaction.deleted_tables      = {{1, 2}};
  compaction.added_tables        = {{3, 4}};

  Json::Value compactionJson = compaction.to_json();

  ObManifestCompaction new_compaction;
  RC                   compaction_rc = new_compaction.from_json(compactionJson);
  EXPECT_EQ(compaction_rc, RC::SUCCESS);
  EXPECT_EQ(new_compaction, compaction);

  // Snapshot
  ObManifestSnapshot snapshot;
  snapshot.seq             = 123;
  snapshot.sstable_id      = 456;
  snapshot.compaction_type = CompactionType::LEVELED;
  snapshot.sstables        = {{1, 2, 3}, {4, 5}, {6}};

  Json::Value json = snapshot.to_json();

  ObManifestSnapshot new_snapshot;
  RC                 rc = new_snapshot.from_json(json);
  EXPECT_EQ(rc, RC::SUCCESS);
  EXPECT_EQ(new_snapshot, snapshot);

  // NewMemtable
  ObManifestNewMemtable memtable;
  memtable.memtable_id = 789;

  Json::Value memtableJson = memtable.to_json();

  ObManifestNewMemtable new_memtable;
  RC                    memtable_rc = new_memtable.from_json(memtableJson);
  EXPECT_EQ(memtable_rc, RC::SUCCESS);
  EXPECT_EQ(new_memtable, memtable);
}

TEST(oblsm_manifest_test, DISABLED_manifest_without_currentfile)
{
  filesystem::remove_all("oblsm_tmp");
  filesystem::create_directory("oblsm_tmp");
  filesystem::path path    = "oblsm_tmp";
  filesystem::path current = path / "CURRENT";
  filesystem::path mf_file = path / "0.mf";

  ObManifest mf{path};
  mf.open();
  EXPECT_EQ(filesystem::exists(current), true);
  EXPECT_EQ(filesystem::exists(mf_file), true);

  std::unique_ptr<ObManifestNewMemtable> memtable_record;
  std::unique_ptr<ObManifestSnapshot>    snapshot;
  std::vector<ObManifestCompaction>      compaction_records;
  mf.recover(snapshot, memtable_record, compaction_records);

  EXPECT_EQ(snapshot, nullptr);
  EXPECT_EQ(memtable_record->memtable_id, 0);
  EXPECT_EQ(compaction_records.empty(), true);

  remove(current.c_str());
  remove(mf_file.c_str());
}

TEST(oblsm_manifest_test, DISABLED_manifest_persist)
{
  filesystem::remove_all("oblsm_tmp");
  filesystem::create_directory("oblsm_tmp");
  filesystem::path path    = "oblsm_tmp";
  filesystem::path current = path / "CURRENT";
  filesystem::path mf_file = path / "0.mf";

  ObManifest mf{path};
  mf.open();
  std::vector<ObManifestCompaction> records;
  int                               count = 3;
  for (int i = 0; i < count; ++i) {
    ObManifestCompaction  r;
    ObManifestSSTableInfo info;
    info.sstable_id = i;
    info.level      = i;
    r.added_tables.push_back(info);
    r.deleted_tables.push_back(info);
    r.sstable_sequence_id = i;
    r.seq_id              = i;
    r.compaction_type     = CompactionType::LEVELED;

    records.push_back(r);
    mf.push(r);
    mf.push(ObManifestNewMemtable{(uint64_t)i});
  }

  std::vector<ObManifestCompaction>      recover_record;
  std::unique_ptr<ObManifestSnapshot>    snapshot;
  std::unique_ptr<ObManifestNewMemtable> memtable;
  mf.recover(snapshot, memtable, recover_record);
  EXPECT_TRUE(recover_record.size() == records.size());
  EXPECT_TRUE(memtable->memtable_id == 2);
  for (size_t i = 0; i < recover_record.size(); ++i) {
    EXPECT_TRUE(recover_record[i] == records[i]);
  }

  remove(current.c_str());
  remove(mf_file.c_str());
}

TEST(oblsm_manifest_test, DISABLED_manifest_reopen)
{
  filesystem::remove_all("oblsm_tmp");
  filesystem::create_directory("oblsm_tmp");
  filesystem::path path    = "oblsm_tmp";
  filesystem::path current = path / "CURRENT";
  filesystem::path mf_file = path / "0.mf";

  ObManifest mf{path};
  mf.open();
  std::vector<ObManifestCompaction> records;
  int                               count = 3;
  for (int i = 0; i < count; ++i) {
    ObManifestCompaction  r;
    ObManifestSSTableInfo info;
    info.sstable_id = i;
    info.level      = i;
    r.added_tables.push_back(info);
    r.deleted_tables.push_back(info);
    r.sstable_sequence_id = i;
    r.compaction_type     = CompactionType::LEVELED;

    records.push_back(r);
    mf.push(r);
    mf.push(ObManifestNewMemtable{(uint64_t)i});
  }

  std::unique_ptr<ObManifestSnapshot>    snapshot;
  std::unique_ptr<ObManifestNewMemtable> memtable;
  std::vector<ObManifestCompaction>      r1, r2;

  mf.recover(snapshot, memtable, r1);

  ObManifest mf_reopen{path};
  mf.recover(snapshot, memtable, r2);

  EXPECT_EQ(r1.size(), r2.size());
  EXPECT_EQ(memtable->memtable_id, 2);

  remove(current.c_str());
  remove(mf_file.c_str());
}

TEST(oblsm_manifest_test, DISABLED_oblsm_recover_empty)
{
  filesystem::remove_all("oblsm_tmp");
  filesystem::create_directory("oblsm_tmp");
  ObLsmOptions options;
  ObLsm       *lsm = nullptr;
  RC           rc  = ObLsm::open(options, "oblsm_tmp", &lsm);
  EXPECT_EQ(rc, RC::SUCCESS);

  delete lsm;

  lsm = nullptr;
  rc = ObLsm::open(options, "oblsm_tmp", &lsm);
  EXPECT_EQ(rc, RC::SUCCESS);
  delete lsm;
}

TEST(oblsm_manifest_test, DISABLED_oblsm_simple_recover)
{
  filesystem::remove_all("oblsm_tmp");
  filesystem::create_directory("oblsm_tmp");
  ObLsmOptions options;
  ObLsm       *lsm = nullptr;
  RC           rc  = ObLsm::open(options, "oblsm_tmp", &lsm);
  EXPECT_EQ(rc, RC::SUCCESS);

  const int count = 10000;
  for (int i = 0; i < count; ++i) {
    std::string key = "tmp_key" + to_string(i);
    lsm->put(key, key);
  }
  sleep(2);
  delete lsm;

  lsm = nullptr;
  ObLsm::open(options, "oblsm_tmp", &lsm);

  ObLsmReadOptions read_options;
  auto             iter = lsm->new_iterator(read_options);
  iter->seek_to_first();
  auto runner = 0;
  while (iter->valid()) {
    // TODO seek key
    std::cout << iter->key() << iter->value() << std::endl;

    iter->next();
    runner++;
  }
  EXPECT_EQ(count, runner);
  delete iter;
  delete lsm;
}



int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}