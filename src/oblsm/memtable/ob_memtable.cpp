/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "oblsm/memtable/ob_memtable.h"
#include "common/lang/string.h"
#include "common/lang/memory.h"
#include "oblsm/util/ob_coding.h"
#include "oblsm/ob_lsm_define.h"

namespace oceanbase {

void ObMemTable::put(uint64_t seq, const string_view &key, const string_view &value)
{
  // TODO: add lookup_key, internal_key, user_key relationship and format in memtable/sstable/block
  // TODO: unify the encode/decode logic in separate file.
  // Format of an entry is concatenation of:
  //  key_size     : internal_key.size()
  //  key bytes    : char[internal_key.size()]
  //  seq          : uint64(sequence)
  //  value_size   : value.size()
  //  value bytes  : char[value.size()]
  size_t       user_key_size          = key.size();
  size_t       val_size          = value.size();
  size_t       internal_key_size = user_key_size + SEQ_SIZE;
  const size_t encoded_len       = sizeof(size_t) + internal_key_size + sizeof(size_t) + val_size;
  char *       buf               = reinterpret_cast<char *>(arena_.alloc(encoded_len));
  char *       p                 = buf;
  memcpy(p, &internal_key_size, sizeof(size_t));
  p += sizeof(size_t);
  memcpy(p, key.data(), user_key_size);
  p += user_key_size;
  memcpy(p, &seq, sizeof(uint64_t));
  p += sizeof(uint64_t);
  memcpy(p, &val_size, sizeof(size_t));
  p += sizeof(size_t);
  memcpy(p, value.data(), val_size);
  table_.insert(buf);
}

int ObMemTable::KeyComparator::operator()(const char *a, const char *b) const
{
  // Internal keys are encoded as length-prefixed strings.
  string_view a_v = get_length_prefixed_string(a);
  string_view b_v = get_length_prefixed_string(b);
  return comparator.compare(a_v, b_v);
}

ObLsmIterator *ObMemTable::new_iterator() { return new ObMemTableIterator(get_shared_ptr(), &table_); }

string_view ObMemTableIterator::key() const { return get_length_prefixed_string(iter_.key()); }

string_view ObMemTableIterator::value() const
{
  string_view key_slice = get_length_prefixed_string(iter_.key());
  return get_length_prefixed_string(key_slice.data() + key_slice.size());
}

void ObMemTableIterator::seek(const string_view &k)
{
  tmp_.clear();
  iter_.seek(k.data());
}

}  // namespace oceanbase