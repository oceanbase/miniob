---
title: 暑期学校2025（实时分析）
---
> 请不要将代码提交到公开仓库（包括提交带有题解的 Pull Request），同时也请不要抄袭其他同学或网络上可能存在的代码。

# 暑期学校2025（实时分析）

本篇文档为中国数据库暑期学校 2025 中 MiniOB 部分实训文档。MiniOB 用 C++ 编写，是一个教学用途的数据库，其目标是为在校学生、数据库从业者、爱好者提供一个友好的数据库学习项目，更好地将理论、实践进行结合，提升同学们的工程实战能力。在本次实训中，需要同学们基于 MiniOB 完成如下四个主题的实验。

* LAB#1 PAX 存储和数据导入
* LAB#2 向量化执行算子实现
* LAB#3 物化视图实现
* LAB#4 实时分析性能优化

## 在实训之前你需要了解的

由于很多同学是第一次接触 MiniOB，可能对实训流程没有概念。所以这里提供一个简单的实训流程，以及一个入门小任务（**不需要写代码**）帮助大家熟悉开发环境和测试平台，并获得本次实训的基础分。

### MiniOB 实训流程

1. **学习 C++ 编程基础。** MiniOB 用 C++ 编写，如果你没有 C++ 编程基础，请先参考本文中 `c++编程语言学习` 的参考资料（或者其他任何资料）学习 C++ 基础知识。可以通过 Cpplings: `src/cpplings/README.md` 来做一个编程练习。
2. **准备自己的代码仓库。** 请参考[文档](../game/github-introduction.md)，在 Github/Gitee 上创建自己的**私有代码**仓库。(**请不要创建公开仓库，也不要将代码提交到公开仓库。**)
3. **准备自己的开发环境。** 我们为大家提供了一个[开源学堂在线编程环境](TODO)，在准备本地开发环境存在困难的情况下可以使用开源学堂在线编程环境进行实验，在线编程环境已经提供了可以直接用于 MiniOB 编程的环境，便于大家快速开始。对于希望在本地准备开发环境的同学，[这篇文档](../dev-env/introduction.md) 中已经介绍的十分详细，请先认真阅读。如果仍有疑问，欢迎提问，也非常欢迎刚刚入门的同学分享自己准备开发环境的经验。
4. **浏览本篇文档的实验部分。** 需要大家理解各个题目描述功能，写代码，实现题目要求的功能。[MiniOB 教程](https://open.oceanbase.com/course/427)中有一些视频讲解教程，可以辅助学习 MiniOB。
5. **提交自己的代码到线上测试平台**(需要使用 git 命令提交代码，可参考[文档](../game/git-introduction.md))，在训练营中提交测试（参考[训练营使用说明](https://ask.oceanbase.com/t/topic/35600372) ）。
6. **继续重复上述 4-5 步骤，直到完成所有实验题目。** 提交代码后，训练营会自动运行你的代码，返回测试结果。你需要根据训练营中返回的日志信息，继续修改自己的代码/提交测试。并不断重复这一过程，直到完成所有实验题目。

### 入门小任务

这里提供给大家一个入门小任务，帮助大家熟悉开发环境和测试平台。这个小任务完全不需要编写任何 C++代码，主要用于给大家熟悉实验环境。

**任务描述**：创建自己的 MiniOB 代码仓库，在开发环境中验证 MiniOB 基础功能，并将代码提交到测试平台通过 `basic` 题目。

#### 1. 学会使用 Git/Github，并创建自己的 MiniOB 代码仓库
在本实验课程中，我们使用 [Git](https://git-scm.com/) 进行代码版本管理（可以理解为用来追踪代码的变化），使用 [GitHub](https://github.com/)/[Gitee](https://gitee.com/) 做为代码托管平台。如果对于 Git/GitHub 的使用不熟悉，可以参考Git/Github 的官方文档。这里也提供了简易的[Git 介绍文档](../game/git-introduction.md)帮助大家了解部分 Git 命令。

如果你已经对 Git/Github 的使用有了一定的了解，可以参考[Github 使用文档](../game/github-introduction.md) 或 [Gitee 使用文档](../game//gitee-instructions.md)来创建自己的 MiniOB 代码仓库（**注意：请创建私有（Private）仓库**）。

#### 2. 准备自己的开发环境
MiniOB 的开发环境需要使用 Linux/MacOS 操作系统，建议使用 Linux 操作系统。我们为大家准备好了一个[开源学堂在线编程环境](./cloudlab_setup.md)（**建议大家优先使用在线编程环境，避免由于自己开发环境问题导致的bug**），除此之外，我们准备了详细的[本地开发环境准备文档](../dev-env/introduction.md)。

#### 3. 在开发环境中构建调试 MiniOB，并验证 MiniOB 的基本功能
在准备好自己的开发环境后，你就可以下载 MiniOB 代码，编译 MiniOB 并运行测试用例，验证 MiniOB 的基本功能。

* 下载 MiniOB 代码，**注意：这里请使用自己的私有仓库地址**
```
git clone https://github.com/oceanbase/miniob.git
```

* 在 MiniOB 代码目录下，运行下面命令来编译 MiniOB
```
bash build.sh debug
```

* 进入 build_debug 目录
```
cd build_debug/
```

* 在 MiniOB 代码目录下，运行下面命令来启动 MiniOB
```
./bin/observer
```

* 打开另一个终端，进入 build_debug 目录，运行下面命令来启动 MiniOB client
```
./bin/obclient
```

* 在 obclient 中分别执行下面的 SQL 语句，并查看输出结果是否符合预期。
```
create table t1 (id int, name char(10));
insert into t1 values (1, 'hello');
select * from t1;
```

* 如果一切顺利，你的终端将会展示如下的结果：
```
$./bin/obclient 

Welcome to the OceanBase database implementation course.

Copyright (c) 2021 OceanBase and/or its affiliates.

Learn more about OceanBase at https://github.com/oceanbase/oceanbase
Learn more about MiniOB at https://github.com/oceanbase/miniob

miniob > create table t1 (id int, name char(10));
SUCCESS
miniob > insert into t1 values (1, 'hello');
SUCCESS
miniob > select * from t1;
id | name
1 | hello
miniob >
```

#### 4. 将代码提交到测试平台，并通过 `basic` 题目

测试平台中的 `basic` 题目是用来验证 MiniOB 的基本功能的（如创建表，插入数据，查询数据等），原始的 MiniOB 代码（**不需要任何代码修改**）就可以通过测试。你需要参考[文档](https://ask.oceanbase.com/t/topic/35600372)来将 MiniOB 代码提交到测试平台进行测试，并通过 `basic` 题目。至此，恭喜你已经顺利熟悉了开发环境和测试平台的使用。

### 注意事项

1. **最重要的一条**：请不要将代码提交到公开仓库（包括提交带有题解的 Pull Request），同时也请不要抄袭其他同学或网络上可能存在的代码。
2. 实验文档中提供了较为详细的指导，请优先仔细阅读文档。文档无法做到囊括所有问题的程度，如果还有问题也请随时提问。对于文档中的**注意** 提示，请认真阅读。

### C++ 编程语言学习

实训需要大家具备一定的 C++ 编程基础，这里推荐几个 C++ 基础学习的链接：

- [LearnCpp](https://www.learncpp.com/)(可以作为教程)
- Cpplings: `src/cpplings/README.md`(可以作为小练习)
- [cppreference](en.cppreference.com)(可以作为参考手册)


## LAB#1 PAX 存储和数据导入
在本实验中，你需要实现 PAX 的存储格式，并支持将 CSV 格式的数据导入到 MiniOB 中。

### 存储模型（Storage Models）

数据库的存储模型规定了它如何在磁盘和内存中组织数据。首先，我们来介绍下三种经典的存储模型。

#### N-ARY Storage Model (NSM)

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

#### Decomposition Storage Model (DSM)

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

#### Partition Attributes Across (PAX)

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

### MiniOB 中 PAX 存储格式

### PAX 存储格式实现

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

#### 实验

实现 PAX 存储格式，需要完成 `src/observer/storage/record/record_manager.cpp` 中 `PaxRecordPageHandler::insert_record`, `PaxRecordPageHandler::get_chunk`, `PaxRecordPageHandler::get_record` 三个函数（标注 `// your code here` 的位置），详情可参考这三个函数的注释。行存格式存储是已经在MiniOB 中完整实现的，实现 PAX 存储格式的过程中可以参考 `RowRecordPageHandler`。

**提示**：对应的训练营测试为 lab1-pax-storage

#### 测试

通过 `unittest/pax_storage_test.cpp` 中所有测试用例。

注意：如果需要运行 `pax_storage_test`，请移除`DISABLED_` 前缀。

### 数据导入实现
需要实现 LOAD DATA 语法，对 CSV 格式的文本文件进行导入。

#### 语法
```
-- 导入普通文件
LOAD DATA INFILE 'file_name' INTO TABLE table_name 
        [ FIELDS 
        [TERMINATED BY 'char']
        [ENCLOSED BY 'char']
        ]
```
参数解释：

- `file_name`：文件路径。格式为 "/\$PATH/\$FILENAME"
- `table_name`：导入数据的表的名称。
- `ENCLOSED BY`：设置导出值的修饰符。
- `TERMINATED BY`：设置导出列的结束符。

示例用法如下：
```sql
load data infile "/data/clickdata/tmp.csv" into table hits fields terminated by "," enclosed by '"';
```

**提示**：可以自行实现 CSV 格式解析或使用外部库的方式实现。

**提示**: 对应的训练营题目为 lab1-basic-load-data

**提示**: 相关实现位于 `src/observer/sql/executor/load_data_executor.h`，只需要考虑导入 PAX 格式的堆表表引擎（默认表引擎）即可。请修改并完善 `src/observer/sql/executor/load_data_executor.cpp` 中的 `load_data` 函数。

一个 csv 格式的示例文件如下，其 `ENCLOSED BY` 为 `"`，`TERMINATED BY` 为 `,`：
```
1,2,"abc","a,ef","435"
3,4,"xxx","a
56","456"
5,6,"yyy","a
78","456"
7,8,"zzz","a90","456"
```

#### 支持 ClickBench 数据集导入
本次实训的最后一个实验需要支持 ClickBench 测试。ClickBench 是 Clickhouse 提供的一个用于分析型数据库的基准测试。在本 Lab 中，需要首先支持 ClickBench 数据集的导入。

主要的工作是实现三个额外的数据类型，支持 DATE，TEXT，BIGINT 三种数据类型。

* DATE: 用于存储日期值，通常表示年、月、日的组合。它不包含时间信息（如小时、分钟、秒）。
* BIGINT: 64位整数类型，8字节存储。
* TEXT: 不定长字符串，text字段的最大长度为65535个字节

关于数据类型的更多信息可参考 MySQL 官方文档。

**提示**: 对应的训练营题目为 lab1-clickbench-load-data

**提示**: 实现 TEXT 类型时可使用 `src/observer/storage/record/lob_handler.h` 进行数据存储和读取。TEXT 类型的内存形式可考虑使用 `src/observer/common/type/string_t.h`. TEXT 类型在 `Column` 中可以使用 `VectorBuffer` 来存储实际字符串。

**提示**: 需要在 `src/observer/common/value.h` 中支持新的数据类型，在 `src/observer/common/type/` 目录下完成新类型的实现。可以参照已有数据类型的实现方式。更多介绍可以参考[文档](miniob-how-to-add-new-datatype.md)

#### 测试
请参考 ClickBench 测试的数据导入部分：
https://github.com/oceanbase/ClickBench/tree/miniob/miniob

```shell
# Start MiniOB
nohup $OBSERVER_BIN/observer -P mysql -s $SOCKET_FILE > /dev/null 2>&1 &

sleep 3
# Load the data

mysql -S $SOCKET_FILE < create.sql

echo "load data infile \"$DATA_FILE\" into table hits fields terminated by \",\" enclosed by '\"';" | mysql -S $SOCKET_FILE
```

## LAB#2 向量化执行算子实现
LAB#2 需要在 MiniOB 中实现向量化执行中的简单聚合（ungrouped aggregation）和分组聚合（grouped aggregation），ORDER BY（排序），LIMIT 等算子。

### 执行模型（processing model）

首先，介绍下执行模型的概念，数据库执行模型定义了在数据库中如何执行一个查询计划。这里简单介绍下两种执行模型：火山模型(Volcano iteration model) 和向量化模型(Vectorization model)。

```
  Volcano iteration model     Vectorization model
                                                  
    ┌───────────┐                ┌───────────┐    
    │           │                │           │    
    │ operator1 │                │ operator1 │    
    │           │                │           │    
    └─┬─────▲───┘                └─┬─────▲───┘    
      │     │                      │     │        
next()│     │ tuple    next_chunk()│     │ chunk/batch 
    ┌─▼─────┴───┐                ┌─▼─────┴───┐    
    │           │                │           │    
    │ operator2 │                │ operator2 │    
    │           │                │           │    
    └─┬─────▲───┘                └─┬─────▲───┘    
      │     │                      │     │        
next()│     │ tuple    next_chunk()│     │ chunk/batch
    ┌─▼─────┴───┐                ┌─▼─────┴───┐    
    │           │                │           │    
    │ operator3 │                │ operator3 │    
    │           │                │           │    
    └───────────┘                └───────────┘    
```

#### 火山模型

在火山模型中，每个查询计划操作符（operator）都实现了一个 `next()` 函数。在每次调用时，操作符要么返回一个元组(tuple)，要么在没有更多元组时返回一个空结果。每个操作符在它的子运算符调用 `next()` 来获取元组，然后对它们进行处理，并向上返回结果。

#### 向量化执行模型

向量化执行模型与火山模型类似，其中每个运算符都有一个生成元组的 `next_chunk()` 方法。然而不同的是，`next_chunk()`方法的每次调用都会获取一组元组而不是仅仅一个元组，这会分摊迭代调用开销。

#### MiniOB 中的实现

在 MiniOB 中已经实现了上述的两种执行模型，可以通过配置项 `execution_mode` 用于区分这两种执行模型。默认使用火山模型，按 `tuple` 迭代。可通过下述 SQL 语句设置执行模型。

将执行模型设置为按 `chunk` 迭代的向量化模型。

```sql
SET execution_mode = 'chunk_iterator';
```

将执行模型设置为按 `tuple` 迭代的火山模型。

```sql
SET execution_mode = 'tuple_iterator';
```

### 向量化执行模型中算子实现

**提示**: 本LAB 中的所有实验均使用 `execution_mode` 为 `chunk_iterator`，存储格式为 `storage format=pax`

#### group by 实现

group by 实现主要位于`src/sql/operator/group_by_vec_physical_operator.cpp`中，这里需要实现分组聚合，即类似 `select a, sum(b) from t group by a`的SQL语句。
group by 实现采用了基于哈希的分组方式，主要涉及到以下几个步骤：
1. 在 `open()` 函数中，通过调用下层算子的`next(Chunk &chunk)` 来不断获得下层算子的输出结果（Chunk）。对从下层算子获得的 Chunk 进行表达式计算，根据分组列计算出分组位置，并将聚合结果暂存在相应的分组位置中。
2. 在 `next(Chunk &chunk)` 函数中，将暂存在哈希表中的聚合结果按 Chunk 格式向上返回。

#### 实验

1. 需要补充 `src/observer/sql/parser/yacc_sql.y` 中 标注`// your code here` 位置的 `aggregation` 和 `group by` 相关的语法。并通过 `src/sql/parser/gen_parser.sh` 生成正确的语法分析代码。
2. 需要完整实现位于 `src/observer/sql/operator/group_by_vec_physical_operator.h` 的group by 算子。
3. 需要实现 `src/observer/sql/expr/aggregate_hash_table.cpp::StandardAggregateHashTable` 中标注 `// your code here` 位置的代码。

#### 测试

需通过 `test/case/test/vectorized-aggregation-and-group-by.test`。

注意1：训练营中的测试采用对MiniOB/MySQL 输入相同的测试 SQL（MiniOB 中建表语句包含 storage format 选项，MySQL 中不包含），对比 MiniOB 执行结果与 MySQL 执行结果的方式进行。训练营中的测试 case 相比 `test/case/test/vectorized-aggregation-and-group-by.test` 更加多样（包含一些随机生成的 SQL），但不存在更加复杂的 SQL。

### ORDER BY / LIMIT 实现
ORDER BY 排序是数据库的一个基本功能，就是将查询的结果按照指定的字段和顺序进行排序。

LIMIT 是一种用于限制查询结果返回行数的关键字。它广泛应用于 SQL 查询中，尤其是在控制输出规模或仅获取部分数据时。

ORDER BY 示例：
```sql
select * from t,t1 where t.id=t1.id order by t.id asc,t1.score desc;
示例中就是将结果按照t.id升序、t1.score降序的方式排序。
```
其中`asc`表示升序排序，`desc`表示降序。如果不指定排序顺序，就是升序，即asc。

LIMIT 示例：
```sql
select * from t,t1 where t.id=t1.id order by t.id asc limit 10;
```

**提示**：ORDER BY 与 LIMIT 算子需要参考 GROUP BY 算子的实现，也可参考 `docs/docs/design/miniob-how-to-add-new-sql.md` 中关于如何添加新的 SQL 语法的文档，实现 `ORDER BY` 子句和 `LIMIT` 子句。

#### 测试

需通过 `test/case/test/vectorized-order-by-and-limit.test`。


## LAB#3 物化视图实现

### 背景
物化视图（MATERIALIZED VIEW）与传统的视图不同，因为它实际上存储了查询结果的数据。简而言之，当您创建一个物化视图时，数据库执行与之关联的 SQL 查询，并将结果集存储在磁盘上。它通过预计算和存储视图的查询结果，减少实时计算，从而提升查询性能并简化复杂查询逻辑，常用于快速报表生成和数据分析场景。

物化视图的数据刷新包含全量刷新和增量刷新。

全量刷新类似于创建物化视图时，每次先清空物化视图中的数据，然后再把查询得到的结果插入到物化视图中。实现中需要等价于truncate table + insert into select。

增量刷新会根据更新周期内更新的数据计算出物化视图增量部分来更新到物化视图中，增量更新的代价比全量更新的代价小很多。

关于OceanBase 中物化视图的实现，可以参考官方文档：https://www.oceanbase.com/docs/common-oceanbase-database-cn-1000000002017308

### MiniOB 中的实现
你需要完整实现创建物化视图/对物化视图进行查询的功能。

#### 语法
创建物化视图：

创建物化视图的 SQL 语句格式如下：

```
CREATE MATERIALIZED VIEW view_name 
    AS select_stmt; 
```

view_name：指定待创建的物化视图的名称。
AS view_select_stmt：用于定义物化视图数据的查询（SELECT）语句。该语句用于从基表（普通表或物化视图）中检索数据，并将结果存储到物化视图中。

创建物化视图示例：

```sql
create materialized view mv_v1 as select count(id), sum(age) from t;
```

查询物化视图：
与查询普通表语法一致。

查询物化视图示例：
```sql
select * from mv_v1;
```

#### 测试
本次实训实验中只需要考虑创建物化视图，查询物化视图的功能，不需要实现物化视图的刷新。

**提示**: 类似create table + insert算子，子算子为select。parser阶段create materialized view stmt
可以带一个select stmt，将创建和插入与查询分开处理。

**提示**: 使用抽象表屏蔽表和视图差异，或者将物化视图当作表处理。

## LAB#4 实时分析性能优化

### 背景介绍

ClickBench 是Clickhouse 提供的分析场景的基准测试，主要测试数据库的实时分析能力。 ClickBench 测试中只包含1张大宽表，100多列，99906797行， 总数据量70GB。 查询覆盖43条SQL。ClickBench 的查询SQL都是单表操作，相对比较简单。

我们使用 ClickBench 作为本次实训的实时分析性能优化题目，考虑到测试的数据量和复杂性，我们对 ClickBench 的数据量和查询 SQL 做一些裁剪之后作为本次实训的测试。

### 实时分析性能优化实现
通过本测试需实现的功能列表：
* 支持数据类型 DATE，TEXT，BIGINT的计算。（例如聚合算子/排序等）
* 支持 LAB#2 中的算子。仅支持 `execution_mode` 为 `chunk_iterator` 即可。
  * 聚合算子
  * GROUP BY 算子
  * ORDER BY 算子
  * LIMIT 算子
  
本 lab 以通过 ClickBench 测试为目标，需要实现的具体功能请结合 ClickBench 测试脚本分析。https://github.com/oceanbase/ClickBench/tree/miniob/miniob


**提示**: 线上评测中也会包含部分正确性测试。

**提示**: 对应的线上训练营为 vldbss-2025: lab4-correctness(正确性测试),vldbss-2025-perf（性能测试），性能测试训练营中返回的分数为测试中包含的所有 SQL 的总执行时间。

**提示**: 不允许通过恶意手段对比赛平台、评估系统和环境进行破坏或取得高分。

#### 通过脚本运行 ClickBench 测试
* 下载 ClickBench 代码。

```
git clone https://github.com/oceanbase/ClickBench.git -b miniob
```
* 下载 ClickBench 数据集并解压。由于 ClickBench 数据集较大，我们只取部分数据集（前 100w 行）进行测试。

```
wget --no-verbose --continue 'https://datasets.clickhouse.com/hits_compatible/hits.tsv.gz'
gzip -d -f hits.tsv.gz
head -n 1000000 hits.tsv > tmp.csv
```
* 运行 ClickBench。

```
cd miniob/
./benchmark.sh ~/miniob/build_release/bin/ /tmp/miniob.sock /data/clickdata/tmp.csv
```

#### 优化提示

1. 当前的存储格式为 PAX 格式，每个页面内存储了所有列的数据。因此如果访问页面的部分列，实际上也会产生整个页面的 IO 访问。因此可以尝试对数据存储格式进行优化，避免不必要的 IO 访问。
2. 对于 ORDER BY + LIMIT 的查询，本质上是一个 TOP-N 计算。因此如果 ORDER BY 语句块后面还有 LIMIT 语句，可以在优化器中进一步优化执行计划，生成 TOP-N SORT 算子，即采用堆排序来计算 TOP-N 的数据，减少冗余的排序计算。
3. 使用 SIMD 指令优化部分计算。通过 SIMD 指令，我们可以优化 MiniOB 向量化执行引擎中的部分批量运算操作，如表达式计算，聚合计算等。关于 SIMD 指令的更多介绍可以参考[文档](./miniob-aggregation-and-group-by.md#simd-介绍)
4. 在存储中维护统计信息，记录页面内元数据（最大值、最小值、COUNT、SUM等）。对于某些查询可直接通过元数据得到页面内的计算结果。
5. 受限于线上评测机的规格，不建议使用多线程来优化性能。
6. ......
