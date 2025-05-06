---
title: MiniOB LSM-Tree 存储引擎
---

# MiniOB LSM-Tree 存储引擎

## LSM-Tree 背景介绍
LSM-Tree 是一种数据结构，可用于存储键值对。LSM-Tree 采用了多层的结构，存储部分可以分为内存和磁盘两个部分。内存中的部分称为 MemTable，磁盘中的部分称为 SSTable(Sorted String Table)。LSM 树通过 Append-Only 的方式提供高效的数据写入，为了优化读取性能，LSM-Tree 通过 Compaction 操作定期重新组织数据。

OceanBase 数据库的存储引擎也是基于 LSM-Tree 架构，将数据分为静态基线数据（放在 SSTable 中）和动态增量数据（放在 MemTable 中）两部分，其中 SSTable 是只读的，一旦生成就不再被修改，存储于磁盘；MemTable 支持读写，存储于内存。数据库 DML 操作插入、更新、删除等首先写入 MemTable，等到 MemTable 达到一定大小时转储到磁盘成为 SSTable。在进行查询时，需要分别对 SSTable 和 MemTable 进行查询，并将查询结果进行归并，返回给 SQL 层归并后的查询结果。同时在内存实现了 Block Cache 和 Row cache，来避免对基线数据的随机读。关于 OceanBase 数据库的更多细节可以参考：https://www.oceanbase.com/docs/oceanbase-database-cn

ObLsm 是一个为教学设计的 LSM-Tree 结构的 Key-Value 存储引擎。ObLsm 本身是一个独立于 MiniOB 的模块，可以独立编译使用。ObLsm 包含了 LSM-Tree 中的关键结构，可以帮助大家学习 LSM-Tree 架构。
MiniOB 中也基于 ObLsm 实现了一个基于 LSM-Tree 的表引擎，可以将表数据以 Key-Value 的格式存储到磁盘。

## ObLsm 存储引擎

下面会对 ObLsm 的各个模块作简单介绍，便于大家对 ObLsm 有一个初步的了解，更多细节可以参考源代码 `src/oblsm`。

### MemTable
MemTable 是一种内存数据结构，用作处理即将到来的操作（insert/delete/update）的缓冲区（buffer）。很多数据结构都可以用于 MemTable 的实现，现有的 LSM-Tree 实现（如 LevelDB/RocksDB）中多采用 SkipList，ObLsm 目前也使用 SkipList 作为 MemTable 的底层数据结构。ObLsm 将 insert/update/delete 都视作一条记录来插入到 MemTable 中。

* insert：将一条记录插入到 MemTable 中。
* update：将一条时间戳更大的记录插入到 MemTable 中。
* delete：将一条 value 为空的记录插入到 MemTable 中。

MemTable 将插入的 Key-Value 编码为如下的记录存储。
```
    ┌───────────┬──────────────┬───────┬──────────────┬──────────────────┐
    │           │              │       │              │                  │
    │key_size(8)│ key(key_size)│ seq(8)│ value_size(8)│ value(value_size)│
    │           │              │       │              │                  │
    └───────────┴──────────────┴───────┴──────────────┴──────────────────┘

```

其中，key_size 和 value_size 分别表示 key+seq 和 value 的长度，seq 表示记录的时间戳。括号中表示占用字节数。

MemTable 的实现位于：`src/oblsm/memtable/`，在代码中，我们将上图中的`key` 称为 user_key，将 `key + seq` 称为 internal_key，将`key_size + key + seq` 称为 lookup_key。

#### SkipList
SkipList（跳表）是用于有序元素序列快速搜索的一个数据结构，SkipList 是一个随机化的数据结构，实质就是一种可以进行二分查找的有序链表。SkipList 在原有的有序链表上面增加了多级索引，通过索引来实现快速查找。跳表不仅能提高搜索性能，同时也可以提高插入和删除操作的性能。它在性能上和红黑树，AVL树不相上下，但是跳表的原理非常简单，实现也比红黑树简单很多。

SkipList 的实现位于：`src/oblsm/memtable/ob_skiplist.h`

#### MemTableIterator
MemTableIterator 提供了一种遍历 MemTable 的机制。它可以按序访问 MemTable 中的所有键值对。

MemTableIterator 的实现位于：`src/oblsm/memtable/ob_memtable.h::ObMemTableIterator`

#### MemTable 转储
MemTable 转储是将内存中的 MemTable 持久化到磁盘上的过程。当 MemTable 达到一定大小时，会被转储为不可变的 SSTable。转储过程通常包括排序数据、生成 SSTable 文件并将其写入磁盘。

转储相关代码位于：`src/oblsm/oblsm_impl.h::ObLsmImpl::try_freeze_memtable`

### SSTable
MemTable 的大小达到限制条件，MemTable 的数据以按顺序被转储到磁盘中，被转储到磁盘中的结构称为（SSTable：Sorted Strings table）。

SSTable 是一种有序的键值对存储结构，它通常包含一个或多个块（block），每个块中包含一组有序的键值对。

SSTable 的存储格式示例如下：
```
   ┌─────────────────┐    
   │    block 1      │◄──┐
   ├─────────────────┤   │
   │    block 2      │   │
   ├─────────────────┤   │
   │      ..         │   │
   ├─────────────────┤   │
   │    block n      │◄┐ │
   ├─────────────────┤ │ │
┌─►│  meta size(n)   │ │ │
│  ├─────────────────┤ │ │
│  │block meta 1 size│ │ │
│  ├─────────────────┤ │ │
│  │  block meta 1   ┼─┼─┘
│  ├─────────────────┤ │  
│  │      ..         │ │  
│  ├─────────────────┤ │  
│  │block meta n size│ │  
│  ├─────────────────┤ │  
│  │  block meta n   ┼─┘  
│  ├─────────────────┤    
└──┼                 │    
   └─────────────────┘    
```

其中，block 表示由若干键值对组成的数据块。block meta 用于存储 block 的元信息，包括 block 的大小、block 的位置信息，block 中的键值对数量等。

SSTable 的实现位于：`src/oblsm/table/`

#### Block
为了提高整体的读写效率，一个sstable文件按照固定大小划分为 Block。每个Block中，目前只存储了键值对数据。
Block 的存储格式如下：
```
      ┌─────────────────┐
      │    entry 1      │◄───┐
      ├─────────────────┤    │
      │    entry 2      │    │
      ├─────────────────┤    │
      │      ..         │    │
      ├─────────────────┤    │
      │    entry n      │◄─┐ │
      ├─────────────────┤  │ │
 ┌───►│  offset size(n) │  │ │
 │    ├─────────────────┤  │ │
 │    │    offset 1     ├──┼─┘
 │    ├─────────────────┤  │
 │    │      ..         │  │
 │    ├─────────────────┤  │
 │    │    offset n     ├──┘
 │    ├─────────────────┤
 └────┤  offset start   │
      └─────────────────┘
```

Block 的主要实现位于 `src/oblsm/table/ob_block.h`

#### SSTableBuilder
用于构造一个 `SSTable`，主要实现位于 `src/oblsm/table/ob_sstable_builder.h`
#### BlockBuilder
用于构造一个 `Block`，主要实现位于 `src/oblsm/table/ob_block_builder.h`

### Compaction
Compaction 是 LSM-Tree 的关键组件，Compaction 会将多个 SSTable 合并为一个或多个新的 SSTable。Compaction 的实现主要位于 `src/oblsm/compaction/`

## 系统恢复
### Manifest
每次 LSM-Tree 进行 Compaction 操作，系统就是一个新的版本，Manifest 组件则需要去记录这组版本更替的 SSTable 变动信息。主要实现位于 `src/oblsm/ob_manifest.h`

Manifest 的记录分为三种:
#### Compaction Record

用于记录每次合并操作时，增加的新的 sstables ， 删除的 sstables，这组操作持久化到磁盘上的最大的时间戳(seq), 以及下一个 sstable 的 id。

#### Snapshot Record

用于记录系统当前的元数据的快照，包括 当前的sstables信息，分层信息，时间戳，下一个sstable的id 。用于提升系统恢复速度，当系统恢复完毕，就会生成一个 snapshot record 写入一个新的 manifest 文件。

####  New Memtable Record 

系统进行 minor compaction 时，会将当前的 memtable 转换成 sstable, 创建一个新的 memtable, 这条记录用于记录新的 memtable 的 id， 恢复系统的时候通过这个 id 找到对应的 WAL 文件，恢复 memtbale。

### WAL
WAL（Write-Ahead Log）是 LSM-Tree 用于恢复系统内存数据结构（Memtable）状态的组件。每次向 Memtable 写入数据时，都会先将数据写入 WAL 日志文件，然后再将数据写入 Memtable。主要实现位于 `oblsm/wal/ob_lsm_wal.h`

#### 日志格式
```
    ┌───────────┬──────────────┬───────┬──────────────┬──────────────────┐
    │           │              │       │              │                  │
    │ seq(8)    │ key_len(8)   │ key   │ val_len(8)   │ val              │
    │           │              │       │              │                  │
    └───────────┴──────────────┴───────┴──────────────┴──────────────────┘
```

#### 写入日志
向 LSM-Tree 中插入数据的时候，系统会先向当前 memtable 对应的 WAL 文件 中写入一条日志，日志格式如上图，然后再写入进 memtable 。WAL 记录了所有的写操作，并且是顺序写入的，有助于提高写入性能。WAL 的结构通常是一个顺序文件，其中每一条记录都代表一个写入操作。

#### Memtable恢复
当 Memtable 被持久化到磁盘时，WAL 文件中的操作会被回放，确保内存中的数据与磁盘上的数据一致。恢复过程通常如下：

1. **读取 WAL 文件**：系统首先读取所有尚未被处理的 WAL 日志记录。
2. **重放日志**：按照 WAL 文件中记录的操作顺序，将数据重新写入 Memtable。
3. **合并数据**：恢复的 Memtable 会与当前存储的数据合并，从而恢复到系统崩溃前的最新状态。
4. **清理 WAL 文件**：旧的 WAL 文件会被清除。

### 系统恢复步骤
每次发生 Compaction 操作，系统会写入一条 Compaction Record 进入当前 Manifest 文件中，特别的，当发送 minor compaction 的时候会写入一条 NewMemtable Record 进入当前 Manifest 文件中。

当 LSM-Tree 启动的时候，会读取目录下的 CURRENT 文件，里面保存当前 Manifest 文件的名称，打开当前 Manifest 文件，读取所有的记录，并按照记录信息恢复。

1. 首先从系统快照（Snapshot）恢复
2. 然后一条条地按照合并信息（Compaction Record） 恢复
3. 根据最新的 NewMemtable Record 的记录找到对应的日志文件(WAL) , 然后从日志中恢复 Memtable
4. 恢复完毕之后，创建一个新的 Manifest 文件，生成系统快照，写入到新的 Manifest 文件中，同时写入一条 NewMemtable Record。

## 基于 ObLsm 的表引擎
MiniOB 基于 ObLsm 模块实现了一个 LSM-Tree 表引擎，用于以 Key-Value 格式存储表数据。表引擎的实现位于：`src/observer/storage/table/lsm_table_engine.h`。

LSM-Tree 表引擎使用方法：
在创建表时指定 engine=lsm，即可使用 LSM-Tree 表引擎。
当不指定engine 或指定 engine=heap，将使用堆表作为表数据的存储方式。

```sql
create table t1 (id int primary key, name varchar(20))engine=lsm;
```

在 MiniOB 中，使用关系型模型来描述表结构，而 LSM-Tree 表引擎将表数据以 Key-Value 的形式存储到磁盘，因此需要提供一种机制来将关系型模型转换为 Key-Value 模型。
目前 MiniOB 中以自增列作为 Key，将行数据以 `Table::make_record` 编码为 Value。通过 [orderedcode](https://github.com/google/orderedcode) 来对 Key 列做编码，使得在编码后 Key 的字典序上比较与 Key 对应的原始序列（目前可以认为只有自增列一列，后续支持主键后，Key 会对应表中的多列）上进行比较具有相同的顺序。

此外，为了在同一个 LSM-Tree 引擎中存储多张表的数据，MiniOB 为每一张表分配一个 TableID，并在 Key 中加入 TableID 作为前缀。

因此，每行数据按照如下规则编码成 (Key, Value) 键值对：

```
Key:   t{TableID}r{AutoIncID}
Value: [col1, col2, col3, col4]
```

## 参考资料

1. [OceanBase](https://www.oceanbase.com/docs/oceanbase-database-cn)
2. [LSM-Tree](https://www.cs.umb.edu/~poneil/lsmtree.pdf)
3. [LevelDB](https://github.com/google/leveldb)
4. [RocksDB](https://github.com/facebook/rocksdb/wiki)
5. [Mini-LSM](https://skyzh.github.io/mini-lsm/)