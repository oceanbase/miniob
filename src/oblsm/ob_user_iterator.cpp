/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "common/lang/memory.h"
#include "oblsm/ob_user_iterator.h"
#include "oblsm/include/ob_lsm_iterator.h"
#include "oblsm/util/ob_comparator.h"
#include "oblsm/ob_lsm_define.h"
#include "oblsm/util/ob_coding.h"

namespace oceanbase {

// a simple warpper for internal iterator
class ObUserIterator : public ObLsmIterator
{
public:
  ObUserIterator(ObLsmIterator *iter, uint64_t seq) : iter_(iter), seq_(seq), valid_(false) {}

  ~ObUserIterator() override = default;

  bool valid() const override { return valid_; }

  void seek_to_first() override
  {
    iter_->seek_to_first();
    if (iter_->valid()) {
      find_next_user_entry(false, &saved_key_);
    } else {
      valid_ = false;
    }
  }

  // TODO: implement seek_to_last
  void seek_to_last() override { valid_ = false; }

  void seek(const string_view &target) override
  {
    put_numeric<uint64_t>(&lookup_key_, target.size() + SEQ_SIZE);
    lookup_key_.append(target.data(), target.size());
    put_numeric<uint64_t>(&lookup_key_, seq_);
    iter_->seek(string_view(lookup_key_.data(), lookup_key_.size()));
    if (iter_->valid()) {
      find_next_user_entry(false, &saved_key_);
    } else {
      valid_ = false;
    }
  }

  void next() override
  {
    saved_key_ = extract_user_key(iter_->key());
    iter_->next();
    if (!iter_->valid()) {
      valid_ = false;
      saved_key_.clear();
      return;
    }
    find_next_user_entry(true, &saved_key_);
  }

  // skipping indicated if to skip current entry.
  // skip indicated current user key.
  void find_next_user_entry(bool skipping, std::string *skip)
  {
    do {
      size_t      curr_seq = extract_sequence(iter_->key());
      string_view user_key = extract_user_key(iter_->key());
      string_view value    = iter_->value();
      if (curr_seq <= seq_) {
        if (value.empty()) {  // for delete
          *skip    = user_key;
          skipping = true;
        } else {  // for insert
          if (skipping && user_comparator_.compare(user_key, *skip) <= 0) {

          } else {
            valid_ = true;
            saved_key_.clear();
            return;
          }
        }
      }
      iter_->next();
    } while (iter_->valid());
    saved_key_.clear();
    valid_ = false;
  }

  string_view key() const override { return extract_user_key(iter_->key()); }

  string_view value() const override { return iter_->value(); }

private:
  // internal iterator, the key is internal key
  unique_ptr<ObLsmIterator> iter_;
  uint64_t                  seq_;
  string                    lookup_key_;
  string                    saved_key_;
  bool                      valid_;
  ObDefaultComparator       user_comparator_;
};

ObLsmIterator *new_user_iterator(ObLsmIterator *iter, uint64_t seq) { return new ObUserIterator(iter, seq); }

}  // namespace oceanbase
