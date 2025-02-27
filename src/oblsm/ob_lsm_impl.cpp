/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "oblsm/ob_lsm_impl.h"

#include "common/log/log.h"
#include "oblsm/include/ob_lsm.h"
#include "oblsm/ob_lsm_define.h"
#include "oblsm/ob_manifest.h"
#include "oblsm/table/ob_merger.h"
#include "oblsm/table/ob_sstable.h"
#include "oblsm/table/ob_sstable_builder.h"
#include "oblsm/util/ob_coding.h"
#include "oblsm/compaction/ob_compaction_picker.h"
#include "oblsm/ob_user_iterator.h"
#include "oblsm/compaction/ob_compaction.h"
#include "oblsm/ob_lsm_define.h"

namespace oceanbase {

ObLsmImpl::ObLsmImpl(const ObLsmOptions &options, const string &path)
    : options_(options), path_(path), mu_(), mem_table_(nullptr), imem_tables_()
{
  mem_table_ = make_shared<ObMemTable>();
  sstables_  = make_shared<vector<vector<shared_ptr<ObSSTable>>>>();
  if (options_.type == CompactionType::LEVELED) {
    sstables_->resize(options_.default_levels);
  }

  // TODO: Check the cpu consumption at idle
  executor_.init("ObLsmBackground", 1, 1, 60 * 1000);
  block_cache_ =
      std::unique_ptr<ObLRUCache<uint64_t, shared_ptr<ObBlock>>>{new ObLRUCache<uint64_t, shared_ptr<ObBlock>>(1024)};
}

RC ObLsm::open(const ObLsmOptions &options, const string &path, ObLsm **dbptr)
{
  RC         rc  = RC::SUCCESS;
  ObLsmImpl *lsm = new ObLsmImpl(options, path);
  *dbptr         = lsm;
  return rc;
}

RC ObLsmImpl::put(const string_view &key, const string_view &value)
{
  // TODO: if put rate is too high, slow down writes is needed.
  // currently, the writes is stopped when the memtable is full.
  LOG_TRACE("begin to put key=%s, value=%s", key.data(), value.data());
  RC rc = RC::SUCCESS;
  // TODO: currenttly the memtable use skiplist as the underlying data structure,
  // and the skiplist concurently write is not thread safe, so we use mutex here,
  // if the skiplist support `insert_concurrently()` interface, can we remove the mutex?
  unique_lock<mutex> lock(mu_);

  // TODO: write to WAL
  uint64_t seq = seq_.fetch_add(1);
  mem_table_->put(seq, key, value);
  size_t mem_size = mem_table_->appro_memory_usage();
  if (mem_size > options_.memtable_size) {
    // Thinking point: here vector is used to store imems,
    // but only one imem is stored at most. Is it possible
    // to store more than one imem and what are the implications
    // of storing more than one imem.
    if (imem_tables_.size() >= 1) {
      cv_.wait(lock);
    }
    // check again after get lock(maybe freeze memtable by another thread)
    if (mem_table_->appro_memory_usage() > options_.memtable_size) {
      try_freeze_memtable();
    } else {
      // if there are multi put threads waiting here, need to notify one thread to
      // continue to write to memtable.
      cv_.notify_one();
    }
  }
  return rc;
}

RC ObLsmImpl::try_freeze_memtable()
{
  RC rc = RC::SUCCESS;
  imem_tables_.emplace_back(mem_table_);
  mem_table_   = make_shared<ObMemTable>();
  auto bg_task = [&]() { this->background_compaction(); };
  int  ret     = executor_.execute(bg_task);
  if (ret != 0) {
    rc = RC::INTERNAL;
    LOG_WARN("fail to execute background compaction task");
  }
  return rc;
}

void ObLsmImpl::background_compaction()
{
  unique_lock<mutex> lock(mu_);
  if (imem_tables_.size() >= 1) {
    shared_ptr<ObMemTable> imem = imem_tables_.back();
    imem_tables_.pop_back();
    lock.unlock();
    cv_.notify_one();
    build_sstable(imem);
    // TODO: trig compaction at more scenarios, for example,
    // seek compaction in
    // leveldb(https://github.com/google/leveldb/blob/578eeb702ec0fbb6b9780f3d4147b1076630d633/db/version_set.cc#L650).
    if (!compacting_) {
      compacting_.store(true);
      try_major_compaction();
      compacting_.store(false);
    }
    return;
  }
}

void ObLsmImpl::try_major_compaction()
{
  unique_lock<mutex>             lock(mu_);
  unique_ptr<ObCompactionPicker> picker(ObCompactionPicker::create(options_.type, &options_));
  unique_ptr<ObCompaction>       picked = picker->pick(sstables_);
  lock.unlock();
  if (picked == nullptr || picked->size() == 0) {
    return;
  }
  vector<shared_ptr<ObSSTable>> results = do_compaction(picked.get());

  SSTablesPtr new_sstables = make_shared<vector<vector<shared_ptr<ObSSTable>>>>();
  lock.lock();
  size_t levels_size        = sstables_->size();
  bool   insert_new_sstable = false;
  auto   find_sstable       = [](const vector<shared_ptr<ObSSTable>> &picked, const shared_ptr<ObSSTable> &sstable) {
    for (auto &p : picked) {
      if (p->sst_id() == sstable->sst_id()) {
        return true;
      }
    }
    return false;
  };

  vector<shared_ptr<ObSSTable>> picked_sstables;
  picked_sstables      = picked->inputs(0);
  const auto &level_i1 = picked->inputs(1);
  if (level_i1.size() > 0) {
    picked_sstables.insert(picked_sstables.end(), level_i1.begin(), level_i1.end());
  }
  // TODO: unify the new sstables logic in all compaction type
  if (options_.type == CompactionType::TIRED) {
    for (int i = levels_size - 1; i >= 0; --i) {
      const vector<shared_ptr<ObSSTable>> &level_i = sstables_->at(i);
      for (auto &sstable : level_i) {
        if (find_sstable(picked_sstables, sstable)) {
          if (!insert_new_sstable) {
            new_sstables->insert(new_sstables->begin(), results);
            insert_new_sstable = true;
          }
        } else {
          new_sstables->insert(new_sstables->begin(), level_i);
          break;
        }
      }
    }
  } else if (options_.type == CompactionType::LEVELED) {
    // TODO: apply the compaction results to sstable
  }

  sstables_ = new_sstables;
  lock.unlock();

  // remove from disk
  for (auto &sstable : picked_sstables) {
    sstable->remove();
  }
  try_major_compaction();
}

vector<shared_ptr<ObSSTable>> ObLsmImpl::do_compaction(ObCompaction *picked) { return {}; }

void ObLsmImpl::build_sstable(shared_ptr<ObMemTable> imem)
{
  unique_ptr<ObSSTableBuilder> tb = make_unique<ObSSTableBuilder>(&default_comparator_, block_cache_.get());

  uint64_t sstable_id = sstable_id_.fetch_add(1);
  tb->build(imem, get_sstable_path(sstable_id), sstable_id);
  unique_lock<mutex> lock(mu_);

  // TODO: unify the build sstable logic in all compaction type
  if (options_.type == CompactionType::TIRED) {
    // TODO: record the changes for tired compaction
    // here we use `level_i` to store `run_i`
    sstables_->insert(sstables_->begin(), {tb->get_built_table()});
  } else if (options_.type == CompactionType::LEVELED) {
    sstables_->at(0).emplace_back(tb->get_built_table());
  }
}

string ObLsmImpl::get_sstable_path(uint64_t sstable_id)
{
  return filesystem::path(path_) / (to_string(sstable_id) + SSTABLE_SUFFIX);
}

RC ObLsmImpl::get(const string_view &key, string *value)
{
  RC                     rc = RC::SUCCESS;
  unique_lock<mutex>     lock(mu_);
  shared_ptr<ObMemTable> mem = mem_table_;

  shared_ptr<ObMemTable> imm = nullptr;
  if (!imem_tables_.empty()) {
    imm = imem_tables_.back();
  }
  vector<shared_ptr<ObSSTable>> sstables;
  for (auto &level : *sstables_) {
    sstables.insert(sstables.end(), level.begin(), level.end());
  }
  lock.unlock();
  string lookup_key;
  put_numeric<uint64_t>(&lookup_key, key.size() + SEQ_SIZE);
  lookup_key.append(key.data(), key.size());
  // TODO: currenttly we use only use the latest seq,
  // we need to use specific seq if oblsm support transaction
  put_numeric<uint64_t>(&lookup_key, seq_.load());

  if (OB_SUCC(mem_table_->get(lookup_key, value))) {
    LOG_INFO("get key from memtable");
  } else if (imm != nullptr && OB_SUCC(imm->get(lookup_key, value))) {
    LOG_INFO("get key from immemtable");
  } else {
    for (auto &sst : sstables) {
      // TODO: sort sstables and return newest value
      if (OB_SUCC(sst->get(lookup_key, value))) {
        break;
      }
      if (rc != RC::NOT_EXIST) {
        LOG_WARN("get key from sstables error: %d", rc);
      }
    }
  }
  return rc;
}

ObLsmIterator *ObLsmImpl::new_iterator(ObLsmReadOptions options)
{
  unique_lock<mutex>     lock(mu_);
  shared_ptr<ObMemTable> mem = mem_table_;

  shared_ptr<ObMemTable> imm = nullptr;
  if (!imem_tables_.empty()) {
    imm = imem_tables_.back();
  }
  vector<shared_ptr<ObSSTable>> sstables;
  for (auto &level : *sstables_) {
    sstables.insert(sstables.end(), level.begin(), level.end());
  }
  lock.unlock();
  vector<unique_ptr<ObLsmIterator>> iters;
  iters.emplace_back(mem->new_iterator());
  if (imm != nullptr) {
    iters.emplace_back(imm->new_iterator());
  }
  for (const auto &sst : sstables) {
    iters.emplace_back(sst->new_iterator());
  }

  return new_user_iterator(new_merging_iterator(&default_comparator_, std::move(iters)), seq_.load());
}

void ObLsmImpl::dump_sstables()
{
  unique_lock<mutex> lock(mu_);
  int                level = sstables_->size();
  for (int i = 0; i < level; i++) {
    cout << "level " << i << endl;
    int level_size = 0;
    for (auto &sst : sstables_->at(i)) {
      cout << sst->sst_id() << ": " << sst->size() << ";";
      level_size += sst->size();
    }
    cout << "level size " << level_size << endl;
  }
}

}  // namespace oceanbase