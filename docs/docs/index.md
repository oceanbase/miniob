---
title: MiniOB 介绍
hide:
  - navigation
  - toc
  - path
---

# MiniOB 介绍

MiniOB 是 [OceanBase](https://github.com/oceanbase/oceanbase) 团队基于华中科技大学数据库课程原型，联合多所高校重新开发的、专为零基础的同学设计的数据库入门学习项目。我们的目标是为在校学生、数据库从业者、爱好者或对基础技术感兴趣的人提供一个友好的数据库学习项目。

MiniOB 整体代码简洁，容易上手，设计了一系列由浅入深的题目，帮助同学们从零基础入门，迅速了解数据库并深入学习数据库内核。MiniOB 简化了许多模块，例如不考虑并发操作、安全特性和复杂的事务管理等功能，以便更好地学习数据库实现原理。我们期望通过 MiniOB 的训练，同学们能够熟练掌握数据库内核模块的功能和协同关系，并具备一定的工程编码能力，例如内存管理、网络通信和磁盘 I/O 处理等, 这将有助于同学在未来的面试和工作中脱颖而出。

# [文档](https://oceanbase.github.io/miniob/)
MiniOB 提供了丰富的设计文档和代码注释。如果在阅读代码过程中遇到不容易理解的内容，并且没有相关的文档或文档描述不清晰，欢迎提出 [issue](https://github.com/oceanbase/miniob/issues)，我们会尽快完善文档。

## 快速上手

为了帮助开发者更好地上手并学习 MiniOB，建议阅读以下内容：

1. [MiniOB 框架介绍](./design/miniob-architecture.md)
2. [如何编译 MiniOB 源码](./how_to_build.md)
3. [如何运行 MiniOB](./how_to_run.md)
4. [使用 GitPod 开发 MiniOB](./dev-env/dev_by_gitpod.md)
5. [doxygen 代码文档](./design/doxy/html/index.html)

为了帮助大家更好地学习数据库基础知识，OceanBase社区提供了一系列教程。更多文档请参考 [MiniOB GitHub Pages](https://oceanbase.github.io/miniob/)。建议学习：

1. [《从0到1数据库内核实战教程》  视频教程](https://open.oceanbase.com/activities/4921877?id=4921946)
2. [《从0到1数据库内核实战教程》  基础讲义](https://github.com/oceanbase/kernel-quickstart)
3. [《数据库管理系统实现》  华中科技大学实现教材](./lectures/index.md)

## 系统架构

![MiniOB 架构](./design/images/miniob-architecture.svg)

其中:

- 网络模块(NET Service)：负责与客户端交互，收发客户端请求与应答；
- SQL解析(Parser)：将用户输入的SQL语句解析成语法树；
- 语义解析模块(Resolver)：将生成的语法树，转换成数据库内部数据结构；
- 查询优化(Optimizer)：根据一定规则和统计数据，调整/重写语法树。(部分实现)；
- 计划执行(Executor)：根据语法树描述，执行并生成结果；
- 存储引擎(Storage Engine)：负责数据的存储和检索；
- 事务管理(MVCC)：管理事务的提交、回滚、隔离级别等。当前事务管理仅实现了MVCC模式，因此直接以MVCC展示；
- 日志管理(Redo Log)：负责记录数据库操作日志；
- 记录管理(Record Manager)：负责管理某个表数据文件中的记录存放；
- B+ Tree：表索引存储结构；
- 会话管理：管理用户连接、调整某个连接的参数；
- 元数据管理(Meta Data)：记录当前的数据库、表、字段和索引元数据信息；
- 客户端(Client)：作为测试工具，接收用户请求，向服务端发起请求。


# [OceanBase 大赛](https://open.oceanbase.com/competition)

全国大学生计算机系统能力大赛（以下简称“大赛”）是由系统能力培养研究专家组发起，全国高等学校计算机教育研究会、系统能力培养研究项目示范高校共同主办、OceanBase 承办，面向高校大学生的全国性数据库大赛。
大赛面向全国爱好数据库的高校学生，以“竞技、交流、成长”为宗旨，搭建基于赛事的技术交流平台，促进高校创新人才培养机制，不仅帮助学生从0开始系统化学习 OceanBase 数据库理论知识，提升学生数据库实践能力，更能帮助学生走向企业积累经验，促进国内数据库人才的发展，碰撞出创新的火花。

OceanBase 初赛基于一套适合初学者实践的数据库实训平台 MiniOB，代码量少，易于上手学习，包含了数据库的各个关键模块，是一个系统性的数据库学习平台。基于该平台设置了一系列由浅入深的题目，以帮助同学们更好"零"基础入门。

为了帮助大家能在大赛中取得好成绩，我们提供了一系列的教程和指导，帮助大家更好地学习数据库基础知识，更好地完成大赛题目。
欢迎大家查看[《从0到1数据库内核实战教程》  视频教程](https://open.oceanbase.com/activities/4921877?id=4921946)，视频中包含了代码框架的介绍和一些入门题目的讲解。
> 由于MiniOB是一个持续演进的产品，视频教程中有些内容会与最新代码有冲突，建议大家参考讲解中的思路。

大赛的初赛是在MiniOB上进行的，同学们可以在前几届的题目上进行提前训练，可以让自己比别人提前一步。大家在日常训练时可以在[MiniOB 训练营](https://open.oceanbase.com/train?questionId=500003) 上提交代码进行测试。

在提交前, 请参考并学习 [训练营使用说明](https://ask.oceanbase.com/t/topic/35600372)。

如果大家在大赛中或使用训练营时遇到一些问题，请先查看[大赛 FAQ](https://ask.oceanbase.com/t/topic/35601465)。
