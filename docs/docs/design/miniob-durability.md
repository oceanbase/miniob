---
title: 持久化
---

# 背景

> 本文介绍 [MiniOB](https://github.com/oceanbase/miniob) 中的 clog 模块是如何工作的。

持久化(Durability) 是事务中非常重要的一个模块，也是最复杂的一个模块，实现持久化才能保证数据不丢失。而持久化同时还要保证事务的原子性与数据完整性。如果对事务的一些概念不太了解，建议先学习了解事务的基本概念，比如学习[事务处理](lectures/lecture-6.md)章节，或者在网上搜索更多资料。

# MiniOB 日志设计与实现
## 实现简介
MiniOB 本身使用堆表保存数据，B+树作为索引基础结构。与传统数据库一样，使用 buffer pool manager 管理堆表与索引数据在内存与磁盘中的存放。我们将表数据文件和索引文件(B+树)，都使用分页模式进行管理，即在磁盘存储时，它是一个大文件，在读取和写入时，都按照固定大小的页来操作。当我们对数据库数据做修改时，不管修改的内容比较多（几十兆字节）还是比较少（几个字节），我们都会按照修改数据内容所在页面为单元进行操作，包括数据在内存中的修改、持久化到磁盘以及记录相关的日志。当我们对文件内容进行修改时，就会先记录日志，称为Writing Ahead Log(WAL)，每次修改后的页面刷新到磁盘之前，都确保对应的日志已经写入到磁盘。

## 如何运行与测试
以mvcc模式启动miniob:

```bash
./bin/observer -f ../etc/observer.ini -s miniob.sock -t mvcc -d disk
```

其中 -d 参数指定了记录日志的方式，disk 表示将日志记录到磁盘中，vacuous 表示不记录日志。

客户端连接做操作，就可以看到 miniob/db/sys/clog 文件在增长。

如何测试日志恢复流程？

observer运行过程中产生了一些日志，这时执行 kill -9 `pidof observer` 将服务端进行强制杀死，然后再使用上面的启动命令将服务端启动起来即可。启动时，就会进入到恢复流程。

可以使用 clog_dump 程序读取日志文件，查看日志内容。

```
(base) build_debug $ ./bin/clog_dump miniob/db/sys/clog/clog_0.log 
begin dump file miniob/db/sys/clog/clog_0.log
lsn=1, size=12, module_id=3:TRANSACTION, operation_type:2:COMMIT, trx_id:1, commit_trx_id: 2
lsn=2, size=12, module_id=0:BUFFER_POOL, buffer_pool_id=1, page_num=1, operation_type=0:ALLOCATE
lsn=3, size=16, module_id=2:RECORD_MANAGER, buffer_pool_id:1, operation_type:0:INIT_PAGE, page_num:1, record_size:20
lsn=4, size=36, module_id=2:RECORD_MANAGER, buffer_pool_id:1, operation_type:1:INSERT, page_num:1, slot_num:0
lsn=5, size=20, module_id=3:TRANSACTION, operation_type:0:INSERT_RECORD, trx_id:3, table_id: 0, rid: PageNum:1, SlotNum:0
lsn=6, size=36, module_id=2:RECORD_MANAGER, buffer_pool_id:1, operation_type:3:UPDATE, page_num:1, slot_num:0
lsn=7, size=12, module_id=3:TRANSACTION, operation_type:2:COMMIT, trx_id:3, commit_trx_id: 4
lsn=8, size=36, module_id=2:RECORD_MANAGER, buffer_pool_id:1, operation_type:1:INSERT, page_num:1, slot_num:1
lsn=9, size=20, module_id=3:TRANSACTION, operation_type:0:INSERT_RECORD, trx_id:5, table_id: 0, rid: PageNum:1, SlotNum:1
```

## 设计与实现
MiniOB 的持久化模块代码放在 clog 目录中。
CLog 的命名取自 [OceanBase](https://github.com/oceanbase/oceanbase) 中的日志模块，全称是 commit log。

### 日志模块设计

**日志接口**

日志模块对外的接口是 `LogHandler`，它定义了日志的写入、读取、恢复等接口。有两个实现模式，一个是 `DiskLogHandler`，一个是 `VacuousLogHandler`。前者是将日志写入到磁盘中，后者是不记录日志。`VacuousLogHandler` 虽然也是当前默认的日志记录方式，但它主要是为了方便测试。我们这里只介绍 `DiskLogHandler`。

**日志写入**

在程序正常运行过程中，调用`DiskLogHandler` 的`append`接口，将日志写入到日志缓冲区(`LogEntryBuffer`)。`DiskLogHandler`会启动一个后台线程，不停地将日志缓冲区中的日志写入到磁盘中。

**日志缓冲**

日志缓冲 `LogEntryBuffer` 的设计很简单，使用一个列表来记录每条日志。这里是一个优化点，感兴趣的同学欢迎提交PR。

**日志文件**

为了防止单个日志文件过大，`DiskLogHandler` 在每个日志文件中存放固定个数的日志，当日志文件满了，会创建新的日志文件。日志文件的命名规则是 `clog_0.log`、`clog_1.log`、`clog_2.log`...。管理日志文件的类是 `LogFileManager`，负责创建文件、枚举日志文件等，`LogFileWriter` 负责将日志写入文件，`LogFileReader` 负责从文件中读取日志。注意，当前没有删除日志文件的接口。

**日志内容**

日志文件中存放的是一条条数据，写入的时候也是一条条写入的，那这一条日志在代码中就是 `LogEntry`。一个 `LogEntry` 包含一个日志头 `LogHeader`。一个日志头包含日志序列号LSN、不包含日志头的数据大小(size)和日志所属模块(module_id)。

日志序列号 LSN: Log Sequence Number，一个单调递增的数字，每生成一条新的日志，就会加1。并且在对应的磁盘文件页面中，也会记录页面对应日志编号。这样从磁盘恢复时，如果某个页面的LSN比当前要重做的日志LSN要小，就需要重做，否则就不需要重做。

日志数据大小 size: 不包含日志头的日志大小，在 `LogEntry` 中，通过 `payload_size` 获取。

日志所属模块 module_id: 持久化模块不仅仅为事务服务，其它的模块，比如B+树，也需要依赖日志来保证数据的完整性和一致性。我们将日志按照模块来划分，在重做时各个模块处理自己的数据。

当前一共有四个模块(参考 `LogModule`)，每个模块负责组织自己的数据内容。在`LogEntry`中都表述为一个二进制数组 `data`，在读取和重放时，每个模块自己负责解析具体的数据。

**系统快照**

我们为了防止日志无限增长，或者减少日志恢复时间，会以不同的方式创建一个系统快照，我们就可以把日志快照之前的日志清除或减少日志恢复时间。
MiniOB的系统快照是在当前没有任何页面更新操作时执行的，其将所有的表相关的文件数据都刷新到磁盘中，然后记录当前日志编号(LSN)，作为一次完整的系统快照，代码可以参考 `Db::sync`。

每次执行完一个DDL任务时，就会执行一次快照操作。
注意，执行快照时（包括DDL），由操作者自己确保当前没有其它任何正在进行的操作，比如插入、删除以及其他的DDL。MiniOB当前并没有做DDL相关的并发控制。

**日志重做**

当系统出现异常，比如coredump、掉电等，重启后，需要读取本次磁盘中的日志来恢复没有写入磁盘文件中的数据。
通常我们会从一个一致性点开始读取日志重做，一致性点就是最新的一次系统快照。
重做的过程比较简单，我们会把每条日志读取出来(`DiskLogHandler::replay`)，按照日志头中的模块来划分执行每个模块的重放接口(`IntegratedLogReplayer::replay`)。

### Buffer Pool 模块的日志
Buffer Pool 模块对页面数据几乎没有修改，除了分配新的页面和释放页面。Buffer Pool会将文件的第一个页面当做元数据页面，记录当前文件大小、页面分配情况等，也就是说Buffer Pool需要记录的日志有两类(`BufferPoolOperation`)：分配页面、释放页面，并且修改的页面都是第一个页面。我们实现了一个辅助类来帮助记录相关的日志 `BufferPoolLogHandler`。

### Record Manager 模块的日志
Record Manager 负责在表普通数据上进行增删改查记录，也就是行数据管理。除了增删改会对页面进行修改，Record Manager还增加了一类特殊的日志，就是初始化空页面。与 Buffer Pool 日志实现类似，Record Manager 的日志实现在 `record_log.h/.cpp` 文件中。

### B+ 树模块的日志
相对于Buffer Pool和Record Manager来说，B+树的日志要复杂很多。因为Buffer Pool和Record Manager每次修改数据时不会超过一个页面，那我们使用一条简单的日志就可以保证数据的完整性。但是B+树不同，B+树的一次修改可能会涉及多个页面。比如一次插入操作，如果叶子节点满了，就需要分裂成两个页面，然后在它们的父节点上插入一个节点，此时父节点也可能会满导致需要分裂。依次类推，一个插入操作可能会影响多个页面。而我们必须要保证在程序运行时和异常宕机重启通过日志恢复后，还能保持一致性，即这个操作完全执行成功，所有受到影响的页面都恢复，或者恢复到这个操作没有执行的状态，相当于没有插入过这条数据。

MiniOB的解决方法很简单。在每次操作过程中，所有更新过的页面在操作结束之前，都不会刷新到磁盘中。并且我们会把所有页面更新的数据都记录到一个日志中，并且在记录日志时，同时记录页面更新前的数据在内存中，即undo日志。这样，如果某次操作中间过程由于某种原因失败了，我们可以通过undo日志将数据都恢复过来。在宕机重启时，我们每次重做一条B+树日志，也可以完整的恢复出一个操作相关联的所有数据。

将一次操作所有的更新都记录在一个日志中，并可以保证一次操作要么全部成功要么全部失败，我们称为一个MiniTransaction，可以参考代码 `BplusTreeMiniTransaction`。在其它数据库中，我们也可以看到类似的概念，比如InnoDB中的mini trans，还有些系统称之为系统事务，与用户事务区分开。

B+树涉及到需要记录日志的类型很多，可以参考 `bplus_tree::LogOperation`。由于每个类型的大小和具体数据都不一样，我们使用各种 `Handler` 辅助类来帮助序列化反序列化日志，可以参考 `LogEntryHandler`及其子类。与Buffer Pool和Record Manager不同，这个类增加了 `rollback` 接口，用于在运行过程中利用Undo数据执行回滚操作。

### 事务日志
相对于前面介绍的几个模块的日志，事务日志是完全不同的一个层面的内容。Buffer Pool 等模块记录日志是为了确保系统底层数据不会出现不一致的情况，是系统“自己”发起的。而事务不同，事务是用户发起的。事务日志相对于前面几个“系统”日志，有几个不同的地方：
- 由用户发起的，除了系统方面的原因需要回滚，提交或回滚由用户控制的；
- 理论上持续时间任意长。用户可能开启一个事务，但是长时间不提交；
- 事务日志不关联具体的页面，而系统日志都与具体的页面数据修改相关；
- 事务的大小不确定。系统日志通常都是很小的，但是事务日志可能会很大，比如用户增加或删除了几千万条数据。

MiniOB 的事务日志并没有记录每次操作具体的数据，而只是记录操作了什么，比如插入了一条数据，会记录在哪个表的哪个位置上插入了一条数据。具体的数据日志是在Record Manager中记录的。也就是说，整个事务日志是一个逻辑日志，而不是物理日志。同时，MiniOB的整个日志系统是分层的，事务日志依赖Record Manager和B+树，而这两个模块会依赖Buffer Pool。

MiniOB 提供了两种事务模型，MVCC(`MvccTrx`)和Vacuous(`VacuousTrx`)。MVCC是多版本并发控制，Vacuous本意是不支持事务，因此也不会记录事务日志。这里仅介绍MVCC模式下的事务日志。

MVCC 相关的日志代码可以参考 `mvcc_trx_log.h/.cpp`。在一个事务中，我们记录用户执行过什么，比如插入一条数据、删除一条数据与提交或回滚事务，操作类型参考 `MvccTrxLogOperation`。每条事务日志会包含一个事务头 `MvccTrxLogHeader`，记录事务ID、操作类型等。在事务提交时，一定会等对应的所有事务日志都写入到磁盘中后，才会返回。

与B+树操作类似，事务也有提交和回滚的概念。但是它们的实现方式不同。通常我们认为，B+树涉及到的数据量比较少，所有操作过的数据，会记录undo日志在内存中，在操作过程中，不会记录日志到磁盘上，在提交时，也不会强制要求将对应的日志刷新到磁盘上。而事务日志不同，因为事务日志的大小不确定、持续时间不确定，我们会在操作执行过程中就会生成一条条日志，通过日志模块写入到缓冲区或者磁盘上。在提交时，为了给用户保证提交的事务不丢失，也必须等日志写入磁盘后再返回。由于在进行过程中就有一部分日志会写入到磁盘上，所以即使事务失败了，我们也会记录一条回滚的日志。

**事务的重做**

由于事务日志中并不真正的记录修改过的数据，只是记录做过什么操作，也只有不完整的事务，才需要做回滚操作。不完整的事务回滚时，将曾经做过的操作执行一遍逆过程即可。完整的事务，不需要做任何动作，因为不管是提交还是回滚，它的动作都已经真实的处理过了。重做的代码可以参考 `MvccTrxLogReplayer` 和事务相关的代码 `MvccTrx::redo`。


### 一些缺陷和遗留问题

**页面丢失的风险**

当前设计的分层设计，事务依赖Record Manager和B+树，Record Manager和B+树依赖Buffer Pool，让整体上看起来更清晰，代码也更简单，但是隐藏了一个问题。各层之间的配合与衔接就会出现一些问题。
比如Record Manager中插入了一条数据，此时我们需要申请一个新的页面。那么在日志中会记录：
1. Buffer Pool分配了一个新的页面；
2. Record Manager初始化了这个页面；
3. Record Manager插入了一条数据。

假设在Record Manager在第2步时失败了，我们就会丢失一个页面。因为第2步的失败，是不会回滚第1步的操作，我们并没有把整个操作当做一个事务来处理。这个问题在B+树中也会出现。

**日志缓冲设计过于简单**

日志缓冲直接放在日志中，没有内存限制，可能会导致内存膨胀。
日志按条缓存，会导致内存碎片，对内存不友好。

**页面原子写入问题**

一个页面不管是8K还是4K，都存在原子写入问题，即我们现在无法保证一个页面完整的刷新到磁盘上。如果一个页面只写一半在磁盘上，会导致无法判断的一致性问题。这个问题在MySQL中也出现过。

**日志文件删除问题**

当前没有删除日志文件的接口，会导致日志文件无限增长。

**写一半的日志**

假设某个日志写入一半的时候停电了，那这个日志在恢复时肯定会失败。如果我们不对这条日志做处理，后面的日志接着文件写，后续这个日志文件就不能再恢复了。通常的处理方法是把这条日志给truncate掉。

# 扩展

- [ARIES: A Transaction Recovery Method Supporting Fine-Granularity Locking and Partial Rollbacks Using Write-Ahead Logging](https://github.com/tpn/pdfs/blob/master/ARIES%20-%20A%20Transaction%20Recovery%20Method%20Supporting%20Fine-Granularity%20Locking%20and%20Partial%20Rollbacks%20Using%20Write-Ahead%20Logging%20(1992).pdf) 该论文提出了ARIES算法，这是一种基于日志记录的恢复算法，支持细粒度锁和部分回滚，可以在数据库崩溃时恢复事务。如果想要对steal/no-force的概念有比较详细的了解，可以直接阅读该论文相关部分。

- [Shadow paging](https://www.geeksforgeeks.org/shadow-paging-dbms/) 除了日志做恢复，Shadow paging 是另外一种做恢复的方法。

- [Aether: A Scalable Approach to Logging](http://www.pandis.net/resources/vldb10aether.pdf) 介绍可扩展日志系统的。

- [InnoDB之REDO LOG](http://catkang.github.io/2020/02/27/mysql-redo.html) 非常详细而且深入的介绍了一下InnoDB的redo日志。

- [B+树恢复](http://catkang.github.io/2022/10/05/btree-crash-recovery.html) 介绍如何恢复B+树。

- [CMU 15445 Logging](https://15445.courses.cs.cmu.edu/spring2023/slides/19-logging.pdf)

- [Principles and realization strategies of multilevel transaction management](https://dl.acm.org/doi/pdf/10.1145/103140.103145) 介绍了多层事务管理的原理和实现策略。
