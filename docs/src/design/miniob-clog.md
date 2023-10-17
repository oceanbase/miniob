本文介绍 [MiniOB](https://github.com/oceanbase/miniob) 中的 clog 模块是如何工作的。

# 背景

持久化(Durability) 是事务中非常重要的一个模块，也是最复杂的一个模块，实现持久化才能保证数据不丢失。而持久化同时还要保证事务的原子性与数据完整性。如果对事务的一些概念不太了解，建议先学习了解事务的基本概念，比如学习[事务处理](lectures/lecture-6.md)章节，或者在网上搜索更多资料。

# MiniOB 中的持久化
## 实现简介
MiniOB 是一个学习使用的数据库，当前也在持续演进，持久化的功能是极不完善的，可以说实现了真正持久化功能的0.0001%。MiniOB 本身使用堆表保存数据，另外还有B+树当做索引，与传统数据库类似，会使用 buffer pool manager 管理堆表与索引数据在内存与磁盘中的存放。Buffer pool manager 会按照页来组织数据，在页上的修改，就会使用WAL(write append logging)记录日志，这里叫做clog。但是clog的实现是非常简化的，可以处理的异常场景也比较有限。希望大家通过这么简化的模块，了解一下数据库的日志与恢复的基本流程。

## 如何运行与测试
以mvcc模式启动miniob:

```bash
./bin/observer -f ../etc/observer.ini -s miniob.sock -t mvcc
```

客户端连接做操作，就可以看到 miniob/db/sys/clog 文件在增长。

如何测试日志恢复流程？
observer运行过程中产生了一些日志，这时执行 kill -9 `pidof observer` 将服务端进行强制杀死，然后再使用上面的启动命令将服务端启动起来即可。启动时，就会进入到恢复流程。

## CLog
CLog 的命名取自 [OceanBase](https://github.com/oceanbase/oceanbase) 中的日志模块，全称是 commit log。

当MiniOB启动时开启了mvcc模式，在运行时，如果有事务数据产生，就会生成日志，每一次操作对应一条日志(CLogRecord)。日志记录了当前操作的内容，比如插入一条数据、删除一条数据。
运行时生成的日志会先记录在内存中(CLogBuffer)，当事务提交时，会将当前的事务以及之前的事务日志刷新到磁盘中。
刷新(写入)日志到磁盘时，并没有开启单独的线程，而是直接在调用刷盘(`CLogManager::sync`)的线程中直接写数据的。由于事务是并发运行的，会存在于多个线程中，因此`CLogBuffer::flush_buffer`做了简单粗暴的加锁控制，一次只有一个线程在刷日志。

对数据库比较了解的同学都知道，事务日志有逻辑日志、物理日志，或者混合类型的日志。那MiniOB的日志是什么？
日志中除了事务操作（提交、回滚）相关的日志，只有插入记录、删除记录两种日志，并且记录了操作的具体页面和槽位，因此算是混合日志。

**如何恢复的？**

在进程启动时，会初始化db对象，db对象会尝试加载日志(当然也是CLog模块干的)，然后遍历这些日志调用事务模块的redo接口，将数据恢复出来。恢复的代码可以参考 `CLogManager::recover`。

**当前的诸多缺陷**

当前CLog仅仅记录redo日志并没有undo日志，也没有记录每个页面的对应的日志编号，因此在使用相关的功能时会有很多限制。
在恢复时，默认是**初始状态**的buffer pool加载起来，然后从日志中恢复数据。当然，如果redo日志中没有提交的事务，可以回滚。最终redo完后，数据也没有问题。但是如果buffer pool中内存更新后刷新到了磁盘，特别是有了没有提交事务的数据，那么这部分数据是无法回滚了。因为每个页面中没有日志编号，不知道自己记录的数据对应的哪个版本。

另外，日志记录时也没有处理各种异常情况，比如日志写一半失败了、磁盘满了，恢复时日志没有办法读取出来。
对于buffer pool中的数据，也没有办法保证一个页面是原子写入的，即一个页面要么都写入成功，要么都写入失败，文件系统没有这个保证，需要从应用层考虑解决这个问题。

**如何实现更完善的日志模块？**

日志不仅要考虑数据页面中数据的恢复，还要考虑一些元数据的操作，以及索引相关的操作。
元数据相关的操作包括buffer pool的管理，比如页面的分配、回收。索引当前使用的是B+树，那就需要考虑B+树页面的分配、回收，以及B+树的分裂、合并等操作。这些操作都需要记录日志，以便在恢复时能够恢复出来。这些操作不能完全对应着某一个事务，需要做一些特殊的处理。

**日志模块的性能瓶颈**

当前的日志实现非常低效，可以认为是单线程串行写日志，而且是每个事务完成都要刷新日志到磁盘，这个过程非常耗时。另外，生成日志的过程也是非常低效的，事务每增加一个操作，并且会追加到日志队列中。

**日志之外？**

日志系统是为了配合数据库做恢复，除了日志模块，还有一些需要做的事情，比如数据库的checkpoint，这个是为了减少恢复时的日志量，以及加快恢复速度。

**工具**

为了帮助定位与调试问题，写了一个简单的日志解析工具，可以在miniob编译后找到二进制文件clog_reader，使用方法如下：

```bash
clog_reader miniob/db/sys/
```

> 注意给的参数不是日志文件，而是日志文件所在的目录，工具会自动找到日志文件并解析。由于CLogFile设计缺陷，这里也不能指定文件名。

# 扩展

- [ARIES: A Transaction Recovery Method Supporting Fine-Granularity Locking and Partial Rollbacks Using Write-Ahead Logging](https://github.com/tpn/pdfs/blob/master/ARIES%20-%20A%20Transaction%20Recovery%20Method%20Supporting%20Fine-Granularity%20Locking%20and%20Partial%20Rollbacks%20Using%20Write-Ahead%20Logging%20(1992).pdf) 该论文提出了ARIES算法，这是一种基于日志记录的恢复算法，支持细粒度锁和部分回滚，可以在数据库崩溃时恢复事务。如果想要对steal/no-force的概念有比较详细的了解，可以直接阅读该论文相关部分。

- [Shadow paging](https://www.geeksforgeeks.org/shadow-paging-dbms/) 除了日志做恢复，Shadow paging 是另外一种做恢复的方法。

- [Aether: A Scalable Approach to Logging](http://www.pandis.net/resources/vldb10aether.pdf) 介绍可扩展日志系统的。

- [InnoDB之REDO LOG](http://catkang.github.io/2020/02/27/mysql-redo.html) 非常详细而且深入的介绍了一下InnoDB的redo日志。

- [B+树恢复](http://catkang.github.io/2022/10/05/btree-crash-recovery.html) 介绍如何恢复B+树。

- [CMU 15445 Logging](https://15445.courses.cs.cmu.edu/spring2023/slides/19-logging.pdf)
