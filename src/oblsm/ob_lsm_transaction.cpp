/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "oblsm/include/ob_lsm_transaction.h"
#include "oblsm/util/ob_comparator.h"
#include "common/lang/memory.h"

namespace oceanbase {

/**
 * @brief merge TrxInnerMapIterator and ObUserIterator
 * @details Merges two iterators of different types into one.
 * If the two iterators have the same key, only
 * produce the key once and prefer the entry from left.
 */
class TrxIterator : public ObLsmIterator
{
public:
  TrxIterator(ObLsmIterator *left, ObLsmIterator *right) : left_(left), right_(right) {}
  ~TrxIterator() override = default;

  bool valid() const override { return false; }
  void seek_to_first() override {}
  void seek_to_last() override {}
  void seek(const string_view &key) override {}
  void next() override {}

  string_view key() const override { return ""; }
  string_view value() const override { return ""; }

private:
  unique_ptr<ObLsmIterator> left_;
  unique_ptr<ObLsmIterator> right_;
};

ObLsmTransaction::ObLsmTransaction(ObLsm *db, uint64_t ts) : db_(db), ts_(ts)
{
  (void)db_;
  (void)ts_;
}

RC ObLsmTransaction::get(const string_view &key, string *value) { return RC::UNIMPLEMENTED; }

RC ObLsmTransaction::put(const string_view &key, const string_view &value) { return RC::UNIMPLEMENTED; }

RC ObLsmTransaction::remove(const string_view &key) { return RC::UNIMPLEMENTED; }

ObLsmIterator *ObLsmTransaction::new_iterator(ObLsmReadOptions options) { return nullptr; }

RC ObLsmTransaction::commit() { return RC::UNIMPLEMENTED; }

RC ObLsmTransaction::rollback() { return RC::UNIMPLEMENTED; }

}  // namespace oceanbase
