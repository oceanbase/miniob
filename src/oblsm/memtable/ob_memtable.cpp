/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */


// MemTable 将插入的 Key-Value 编码为如下的记录存储。
//     ┌───────────┬──────────────┬───────┬──────────────┬──────────────────┐
//     │           │              │       │              │                  │
//     │key_size(8)│ key(key_size)│ seq(8)│ value_size(8)│ value(value_size)│
//     │           │              │       │              │                  │
//     └───────────┴──────────────┴───────┴──────────────┴──────────────────┘
// 其中，key_size 和 value_size 分别表示 key+seq 和 value 的长度，seq 表示记录的时间戳。括号中表示占用字节数。
// MemTable 的实现位于：src/oblsm/memtable/，在代码中，
// 我们将上图中的key 称为 user_key，
// 将 key + seq 称为 internal_key，
// 将key_size + key + seq 称为 lookup_key。


#include "oblsm/memtable/ob_memtable.h"
#include "common/lang/string.h"
#include "common/lang/memory.h"
#include "oblsm/util/ob_coding.h"
#include "oblsm/ob_lsm_define.h"

namespace oceanbase {

  // 将一个 (key, value) 以及版本号 seq 插入到内存表 MemTable 中。
  // 示例：
  // key   = "apple"            // 长度 = 5
  // value = "red fruit"        // 长度 = 9
  // seq   = 123456789ULL       // 一个版本号
  // [key_size (8 bytes)] [key (user_key)] [seq (8 bytes)] [value_size (8 bytes)] [value]
  // [      5           ] [apple         ] [123456789    ] [9                   ] [red fruit]
  // 8字节  5字节   8字节      8字节  9字节
  // 8+5    apple  123456789  9      red fruit
  // 表示这些数据需要占据的长度
  // encoded_len  = sizeof(size_t) + 8+5 + sizeof(size_t) + 9; 

void ObMemTable::put(uint64_t seq, const string_view &key, const string_view &value)
{
  std::lock_guard<std::mutex> lock(table_mutex_); // 保护整个put操作
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
  memcpy(p, &internal_key_size, sizeof(size_t)); // 8 字节
  p += sizeof(size_t);
  memcpy(p, key.data(), user_key_size);
  p += user_key_size;
  memcpy(p, &seq, sizeof(uint64_t)); // 8 字节
  p += sizeof(uint64_t);
  memcpy(p, &val_size, sizeof(size_t)); // 8 字节
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




// 📦 SSTable 的存储结构详解
// 我们来拆解你贴的结构图，帮助你理解 SSTable 是如何组织数据的：

// scss
// 复制代码
//    ┌─────────────────┐    
//    │    block 1      │◄──┐
//    ├─────────────────┤   │
//    │    block 2      │   │
//    ├─────────────────┤   │
//    │      ..         │   │
//    ├─────────────────┤   │
//    │    block n      │◄┐ │
//    ├─────────────────┤ │ │
// ┌─►│  meta size(n)   │ │ │
// │  ├─────────────────┤ │ │
// │  │block meta 1 size│ │ │
// │  ├─────────────────┤ │ │
// │  │  block meta 1   ┼─┼─┘
// │  ├─────────────────┤ │  
// │  │      ..         │ │  
// │  ├─────────────────┤ │  
// │  │block meta n size│ │  
// │  ├─────────────────┤ │  
// │  │  block meta n   ┼─┘  
// │  ├─────────────────┤    
// └──┼                 │    
//    └─────────────────┘    
// 我们从上到下看：

// 1. Block 区域（数据部分）
// │ block 1 │
// │ block 2 │
// │  ...    │
// │ block n │
// 2. Meta 信息区域（索引/元信息）
// 紧跟在所有 block 后面，是描述这些 block 的元信息区域：
// │  meta size(n)     │
// │ block meta 1 size │
// │ block meta 1      │
// │ block meta 2 size │
// │ block meta 2      │
// │       ...         │
// │ block meta n size │
// │ block meta n      │
// 这些是描述 block 的"目录"，类似于书的目录页：
// meta size(n)：说明一共有多少个 block。
// block meta i size：说明第 i 个 block 的元信息有多大。
// block meta i：包含该 block 的各种元数据，例如：
// block 在 SSTable 文件中的起始偏移（offset）
// block 的大小
// block 中 key 的范围（起始 key、结束 key）