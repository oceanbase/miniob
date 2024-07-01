---
title: PAX 存储格式
---

# MiniOB PAX 存储格式

本篇文档介绍 MiniOB 中 PAX 存储格式。

## 存储模型（Storage Models）

数据库的存储模型规定了它如何在磁盘和内存中组织数据。首先，我们来介绍下三种经典的存储模型。

### N-ARY Storage Model (NSM)

在 NSM 存储模型中，一行记录的所有属性连续存储在数据库页面（Page）中，这也被称为行式存储。NSM 适合 OLTP 工作负载。因为在 OLTP 负载中，查询更有可能访问整个记录（对整个记录进行增删改查）。

```
     Col1 Col2 Col3                   Page          
    ┌─────────────┬─┐        ┌──────────┬──────────┐
Row1│ a1   b1   c1│ │        │PageHeader│ a1 b1 c1 │
    ├─────────────┤ │        ├────────┬─┴──────┬───┤
Row2│ a2   b2   c2│ │        │a2 b2 c2│a3 b3 c3│.. │
    ├─────────────┤ │        ├────────┴────────┴───┤
Row3│ a3   b3   c3│ │        │...                  │
    ├─────────────┘ │        ├─────────────────────┤
... │ ..   ..   ..  │        │...                  │
... │ ..   ..   ..  │        └─────────────────────┘
    │               │                               
RowN│ an   bn   cn  │                               
    └───────────────┘ 
```

### Decomposition Storage Model (DSM)

在 DSM 存储模型中，所有记录的单个属性被连续存储在数据块/文件中。这也被称为列式存储。DSM 适合 OLAP 工作负载，因为 OLAP 负载中往往会对表属性的一个子集执行扫描和计算。

```
                                 File/Block       
     Col1 Col2 Col3        ┌─────────────────────┐
    ┌────┬────┬────┬┐      │ Header              │
Row1│ a1 │ b1 │ c1 ││      ├─────────────────────┤
    │    │    │    ││      │a1 a2 a3 ......... an│
Row2│ a2 │ b2 │ c2 ││      └─────────────────────┘
    │    │    │    ││                             
Row3│ a3 │ b3 │ c3 ││      ┌─────────────────────┐
    │    │    │    ││      │ Header              │
    │  . │....│.   ││      ├─────────────────────┤
... │    │    │    ││      │b1 b2 b3 ......... bn│
... │ .. │ .. │ .. ││      └─────────────────────┘
RowN│ an │ bn │ cn ││                             
    └────┴────┴────┴┘      ┌─────────────────────┐
                           │ Header              │
                           ├─────────────────────┤
                           │c1 c2 c3 ......... cn│
                           └─────────────────────┘
```

### Partition Attributes Across (PAX)

PAX (Partition Attributes Across) 是一种混合存储格式，它在数据库页面（Page）内对属性进行垂直分区。

```
     Col1 Col2 Col3                   Page          
    ┌─────────────┬─┐        ┌──────────┬──────────┐
Row1│ a1   b1   c1│ │        │PageHeader│ a1 a2 a3 │
    │             │ │        ├──────────┼──────────┤
Row2│ a2   b2   c2│ │        │b1 b2 b3  │ c1 c2 c3 │
    │             │ │        └──────────┴──────────┘
Row3│ a3   b3   c3│ │                 ....          
    ├─────────────┘ │        ┌──────────┬──────────┐
    │  .........    │        │PageHeader│ ..... an │
... ├─────────────┐ │        ├──────────┼──────────┤
... │ ..   ..   ..│ │        │...... bn │ ..... cn │
RowN│ an   bn   cn│ │        └──────────┴──────────┘
    └─────────────┴─┘                               
```

## MiniOB 中 PAX 存储格式

### 实现

在 MiniOB 中，RecordManager 负责一个文件中表记录（Record）的组织/管理。在没有实现 PAX 存储格式之前，MiniOB 只支持行存格式，每个记录连续存储在页面（Page）中，通过`RowRecordPageHandler` 对单个页面中的记录进行管理。需要通过实现 `PaxRecordPageHandler` 来支持页面内 PAX 存储格式的管理。
Page 内的 PAX 存储格式如下：
```
| PageHeader | record allocate bitmap | column index  |
|------------|------------------------| ------------- |
| column1 | column2 | ..................... | columnN |
```
其中 `PageHeader` 与 `bitmap` 和行式存储中的作用一致，`column index` 用于定位列数据在页面内的偏移量，每列数据连续存储。

`column index` 结构如下，为一个连续的数组。假设某个页面共有 `n + 1` 列，分别为`col_0, col_1, ..., col_n`，`col_i` 表示列 ID（column id）为 `i + 1`的列在页面内的起始地址(`i < n`)。当 `i = n`时，`col_n` 表示列 ID 为 `n` 的列在页面内的结束地址 + 1。
```
| col_0 | col_1 | col_2 | ......  | col_n |
|-------|-------|-------|---------|-------|
```


MiniOB 支持了创建 PAX 表的语法。当不指定存储格式时，默认创建行存格式的表。
```
CREATE TABLE table_name
      (table_definition_list) [storage_format_option]

storage_format_option:
      storage format=row
    | storage format=pax
```
示例：

创建行存格式的表：

```sql
create table t(a int,b int) storage format=row;
create table t(a int,b int);
```

创建列存格式的表：
```sql
create table t(a int,b int) storage format=pax;
```

### 实验

实现 PAX 存储格式，需要完成 `src/observer/storage/record/record_manager.cpp` 中 `PaxRecordPageHandler::insert_record`, `PaxRecordPageHandler::get_chunk`, `PaxRecordPageHandler::get_record` 三个函数（标注 `// your code here` 的位置），详情可参考这三个函数的注释。行存格式存储是已经在MiniOB 中完整实现的，实现 PAX 存储格式的过程中可以参考 `RowRecordPageHandler`。

### 测试

通过 `unittest/pax_storage_test.cpp` 中所有测试用例，通过`benchmark/pax_storage_concurrency_test.cpp` 性能测试。

注意：如果需要运行 `pax_storage_test` 、`pax_storage_concurrency_test`，请移除`DISABLED_` 前缀。

