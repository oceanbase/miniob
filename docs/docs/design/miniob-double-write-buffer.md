---
title: Double Write Buffer 实现
---

# MiniOB Double Write Buffer 实现

## 当前MiniOB的问题

因为buffer内的数据页比文件系统的数据页要大，所以在进行刷盘操作时，可能会发生如下场景：

当数据库准备刷新脏页时，需要多次IO才能加数据页全部写入磁盘。

但当执行完第一次IO时，数据库发生意外宕机，导致此时才刷了一个文件系统里的页，这种情况被称为写失效（partial page write）。

此时重启后，磁盘上的页就是不完整的数据页，就算使用redo log也无法进行恢复。

## Double Write Buffer解决的问题

在数据库进行脏页刷新时，如果此时宕机，有可能会导致磁盘数据页损坏，丢失我们重要的数据。此时就算重做日志也是无法进行恢复的，因为重做日志记录的是对页的物理修改。

Double Write其实就是在重做日志前，用户需要一个页的副本，当写入失效发生时，先通过页的副本来还原该页，再进行重做，这就是double write。

## Double Write Buffer架构

![Double Write Buffer](images/miniob-double-write-buffer-struct.png)

## Double Write Buffer 工作流程

1. **添加页面** ：当buffer pool要刷脏页时，不直接写磁盘，而是把脏页添加到double write buffer中，double write buffer再将页面写入共享表空间。
2. **读取页面** ：当buffer pool要读取页面时，先查看double write buffer中是否存在该页面，若存在，则直接拷贝，若不存在，则从磁盘中读取页面。
3. **写入页面** ：当double write buffer装满时，double write buffer会先同步磁盘，防止缓冲写带来的问题。再将页面写入对应的数据文件中。
4. **崩溃恢复** ：当数据库重启恢复时，先读取共享表空间中的文件，若该页面未损坏，则直接拷贝至磁盘中对应的数据页面，若页面损坏，则忽略该页面。此举可以保证数据库在恢复时数据文件中的页面是完好的。

## Double Write Buffer的问题

Double write buffer 它是在物理文件上的一个buffer, 其实也就是file，所以它会导致系统有更多的fsync操作，而因为硬盘的fsync性能问题，所以也会影响到数据库的整体性能。
