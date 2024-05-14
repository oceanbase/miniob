---
title: MiniOB架构
---

# MiniOB代码架构框架设计和说明

## MiniOB代码结构说明

### 背景
MiniOB旨在帮助不太熟悉数据库设计和实现的同学快速掌握和深入学习数据库内核。我们希望通过MiniOB的培训，学生能够理解不同数据库内核模块的功能和它们之间的联系。这个项目主要面向在校学生，对模块的设计和实现进行了简化处理，以便于他们更好地理解和学习。

### MiniOB架构介绍

![MiniOB架构](./images/miniob-architecture.svg)

- 网络模块(NET Service)：负责与客户端交互，收发客户端请求与应答；
- SQL解析(Parser)：将用户输入的SQL语句解析成语法树；
- 语义解析模块(Resolver)：将生成的语法树，转换成数据库内部数据结构；
- 查询优化(Optimizer)：根据一定规则和统计数据，调整/重写语法树；
- 计划执行(Executor)：根据语法树描述，执行并生成结果；
- 存储引擎(Storage Engine)：负责数据的存储和检索；
- 事务管理(MVCC)：管理事务的提交、回滚、隔离级别等。当前事务管理仅实现了MVCC模式，因此直接以MVCC展示；
- 日志管理(Redo Log)：负责记录数据库操作日志；
- 记录管理(Record Manager)：负责管理某个表数据文件中的记录存放；
- B+ Tree：表索引存储结构；
- 会话管理：管理用户连接、调整某个连接的参数；
- 元数据管理(Meta Data)：记录当前的数据库、表、字段和索引元数据信息；
- 客户端(Client)：作为测试工具，接收用户请求，向服务端发起请求。


### 各模块工作原理介绍

#### 服务端启动过程

虽然代码是模块化的，并且面向对象设计思想如此流行，但是很多同学还是喜欢从main函数看起。那么就先介绍一下服务端的启动流程。

main函数参考 main@src/observer/main.cpp。启动流程大致如下：

解析命令行参数 parse_parameter@src/observer/main.cpp

加载配置文件    Ini::load@deps/common/conf/ini.cpp

初始化日志       init_log@src/observer/init.cpp

初始化网络服务 init_server@src/observer/main.cpp

启动网络服务    Server::serve@src/net/server.cpp

建议把精力更多的留在核心模块上，以更快的了解数据库的工作原理。

#### 网络模块
网络模块代码参考src/observer/net，主要是Server类。
当前支持TCP socket和Unix socket，TCP socket可以跨主机通讯，需要服务端监听特定端口。Unix socket只能在本机通讯，测试非常方便。
在处理具体连接的网络IO请求时，会有具体的线程模型来处理，当前支持一对一连接线程模型和线程池模型，可以参考文档[MiniOB线程模型](./miniob-thread-model.md)。
网络服务启动后，会监听端口(TCP)或Unix连接，当接收到新的连接，会将新的连接描述字加入网络线程模型中。
线程模型会在进程运行时持续监听对应socket上新请求的到达，然后将请求交给具体的处理模块。

#### SQL解析
SQL解析模块是接收到用户请求，开始正式处理的第一步。它将用户输入的数据转换成内部数据结构，一个语法树。
解析模块的代码在`src/observer/sql/parser`下，其中`lex_sql.l`是词法解析代码，`yacc_sql.y`是语法解析代码，`parse_defs.h`中包含了语法树中各个数据结构。
对于词法解析和语法解析，原理概念可以参考《编译原理》。
其中词法解析会把输入（这里比如用户输入的SQL语句）解析成成一个个的“词”，称为token。解析的规则由自己定义，比如关键字SELECT，或者使用正则表达式，比如`"[A-Za-z_]+[A-Za-z0-9_]*"` 表示一个合法的标识符。
对于语法分析，它根据词法分析的结果（一个个token），按照编写的规则，解析成“有意义”的“话”，并根据这些参数生成自己的内部数据结构。比如`SELECT * FROM T`，可以据此生成一个简单的查询语法树，并且知道查询的`columns`是"*"，查询的`relation`是"T"。
NOTE：在查询相关的地方，都是用关键字relation、attribute，而在元数据中，使用table、field与之对应。

更多相关内容请参考 [MiniOB SQL语法解析](./miniob-sql-parser.md)。

#### 语义解析
语法解析会将用户发来的SQL文本内容，解析为一个文本描述的语法树，语义解析(Resolver)将语法树中的一些节点，比如表名、字段名称等，转换为内部数据结构中的真实对象。

解析可以做的更多，比如在解析表字段映射的过程中，可以创建Tuple，将字段名直接转换为使用更快的Field或数字索引的方式来访问某一行数据的字段。当前没有做此优化，每次都是在执行过程中根据字段名字来查找特定的字段，参考类 `ProjectTuple`。

#### 优化
优化决定SQL执行效率非常重要的一环，通常会根据一定的规则，对SQL语法树做等价调整，再根据一些统计数据，比如表中的数据量、索引情况等，来选择更好的执行计划。
MiniOB中的执行计划优化仅实现了简单的框架，可以参考 `OptimizeStage`。

#### 计划执行
顾名思义，计划执行就是按照优化后生成的执行计划原意执行，获取SQL结果。
当前查询语句被转换成了火山执行计划，执行时按照火山模型算子中，通过执行算子的 `next` 方法获取每行的执行结果。
对于DDL等操作，SQL最终被转换为各种Command，由`CommandExecutor`来执行。
计划执行的代码在`src/observer/sql/executor/`下，主要参考`execute_stage.cpp`的实现。

#### 元数据管理模块
元数据是指数据库一些核心概念，包括db、table、field、index等，记录它们的信息。比如db，记录db文件所属目录；field，记录字段的类型、长度、偏移量等。代码文件分散于`src/observer/storage/table,field,index`中，文件名中包含`meta`关键字。

#### 客户端
这里的客户端提供了一种测试miniob的方法。从标准输入接收用户输入，将请求发给服务端，并展示返回结果。这里简化了输入的处理，用户输入一行，就认为是一个命令。

#### 通信协议
MiniOB 采用TCP通信，支持两种通讯协议，分别是纯文本模式和MySQL通讯协议，详细设计请参考 [MySQL 通讯协议设计](./miniob-mysql-protocol.md)。
对于纯文本模式，客户端与服务端发送数据时，使用普通的字符串来传递数据，使用'\0'字符作为每个消息的终结符。

注意：测试程序也使用这种方法，***请不要修改协议，后台测试程序依赖这个协议***。
注意：返回的普通数据结果中不要包含'\0'，也不支持转义处理。

为了方便测试，MiniOB 支持不使用客户端，可以直接启动后在终端输入命令的方式做交互，在启动 observer 时，增加 `-P cli` 参数即可，更多信息请参考 [如何运行MiniOB](../how_to_run.md)。

## 参考
- 《数据库系统概念》
- 《数据库系统实现》
- 《flex_bison》  flex/bison手册
- [flex开源源码](https://github.com/westes/flex)
- [bison首页](https://www.gnu.org/software/bison/)
- [cmake官方手册](https://cmake.org/)
- [libevent官网](https://libevent.org/)
- [OceanBase数据库文档](https://www.oceanbase.com/docs)
- [OceanBase开源网站](https://github.com/oceanbase/oceanbase)

## 附录-编译安装测试

### 编译
参考 [如何构建MiniOB](../how_to_build.md) 文件。

### 运行服务端
参考 [如何运行MiniOB](../how_to_run.md)。
