---
title: 数据库系统实现原理与实践课程实验
---

> 请不要将代码提交到公开仓库（包括提交带有题解的 Pull Request），同时也请不要抄袭其他同学或网络上可能存在的代码。

# 数据库系统实现原理与实践课程实验

这是 OceanBase 团队与华东师范大学联合开发的数据库系统实现原理与实践课程的配套实验课程，课程学习请参考：https://www.shuishan.net.cn/mooc/course/1888745117930643457。本课程中所有的实验题目都将基于 [MiniOB](https://github.com/oceanbase/miniob) 完成。MiniOB 用 C++ 编写，是一个教学用途的数据库，其目标是为在校学生、数据库从业者、爱好者提供一个友好的数据库学习项目，更好地将理论、实践进行结合，提升同学们的工程实战能力。

本课程在学生了解数据库使用的前提下，学习数据管理系统(以关系数据库为主)在实现上的原理与关键技术。重点学习数据库系统的体系结构，包括数据库存储、查询、事务处理与数据库前沿知识四个方面，以及深入各方面包括如数据库索引，查询优化，并发控制，日志管理，等关键的原理与技术，为后续从事数据管理系统的研究和工程打下理论与实践基础，了解数据系统在设计与实践方面的分析、思维与学习方法。

该课程的实验项目完全公开，适合学习系统编程技能的高年级本科生，欢迎对数据库系统实现原理与实践感兴趣的同学参与学习。

## 实验题目
这是本课程的实验题目，实验题目是修改 MiniOB 的各个组件并完成指定的功能，实验题目会持续更新。

- [LAB#0 C++ 基础入门](./lab0.md)
- [LAB#1 LSM-Tree 存储引擎](./lab1.md)
- [LAB#2 查询引擎](./lab2.md)
- [LAB#3 事务引擎](./lab3.md)
- [LAB#4 性能测试](./lab4.md)

## 零基础入门数据库系统实现原理与实践课程

由于很多同学是第一次接触 MiniOB，第一次接触 Github/Git/Docker/Vscode 等新事物，可能对实验流程完全没有概念。所以这里提供一个简单的实验流程，以及一个入门小任务（**不需要写代码**）帮助大家熟悉开发环境和测试平台，并获得本课程实验的基础分。

### 实验流程

1. **学习 C++ 编程基础。** MiniOB 用 C++ 编写，如果你没有 C++ 编程基础，请先参考本文中 `c++编程语言学习` 的参考资料（或者其他任何资料）学习 C++ 基础知识。可以通过 Cpplings: `src/cpplings/README.md` 和 [LAB#0](./lab0.md) 来做一个编程练习。
2. **准备自己的代码仓库。** 请参考[文档](../game/github-introduction.md)，在 Github/Gitee 上创建自己的**私有代码**仓库。(**请不要创建公开仓库，也不要将代码提交到公开仓库。**)
3. **准备自己的开发环境。** 我们为大家提供了一个[开源学堂在线编程环境](../dev-env/cloudlab_setup.md)，推荐大家使用开源学堂在线编程环境进行实验，在线编程环境已经提供了可以直接用于 MiniOB 编程的环境，便于大家快速开始。对于希望在本地准备开发环境的同学，[这篇文档](../dev-env/introduction.md) 中已经介绍的十分详细，请先认真阅读。如果仍有疑问，欢迎提问，也非常欢迎刚刚入门的同学分享自己准备开发环境的经验。
4. **浏览实验手册中的文档。** 实验手册文档位于本目录下，需要大家理解各个题目描述功能，写代码，实现题目要求的功能。[MiniOB 教程](https://open.oceanbase.com/course/427)中有一些视频讲解教程，可以辅助学习 MiniOB。
5. **提交自己的代码到线上测试平台**(需要使用 git 命令提交代码，可参考[文档](../game/git-introduction.md))，在训练营中提交测试（参考[训练营使用说明](https://ask.oceanbase.com/t/topic/35600372) ）。
6. **继续重复上述 4-5 步骤，直到完成所有实验题目。** 提交代码后，训练营会自动运行你的代码，返回测试结果。你需要根据训练营中返回的日志信息，继续修改自己的代码/提交测试。并不断重复这一过程，直到完成所有实验题目。此时，恭喜你，本门课程的实验课程你拿到了满分。

### 入门小任务

这里提供给大家一个入门小任务，帮助大家熟悉开发环境和测试平台。这个小任务完全不需要编写任何 C++代码，主要用于给大家熟悉实验环境。

**任务描述**：创建自己的 MiniOB 代码仓库，在开发环境中验证 MiniOB 基础功能，并将代码提交到测试平台通过 `basic` 题目。

#### 1. 学会使用 Git/Github，并创建自己的 MiniOB 代码仓库
在本实验课程中，我们使用 [Git](https://git-scm.com/) 进行代码版本管理（可以理解为用来追踪代码的变化），使用 [GitHub](https://github.com/)/[Gitee](https://gitee.com/) 做为代码托管平台。如果对于 Git/GitHub 的使用不熟悉，可以参考Git/Github 的官方文档。这里也提供了简易的[Git 介绍文档](../game/git-introduction.md)帮助大家了解部分 Git 命令。

如果你已经对 Git/Github 的使用有了一定的了解，可以参考[Github 使用文档](../game/github-introduction.md) 或 [Gitee 使用文档](../game//gitee-instructions.md)来创建自己的 MiniOB 代码仓库（**注意：请创建私有（Private）仓库**）。

#### 2. 准备自己的开发环境
MiniOB 的开发环境需要使用 Linux/MacOS 操作系统，建议使用 Linux 操作系统。我们为大家准备好了一个[开源学堂在线编程环境](../dev-env/cloudlab_setup.md)（**建议大家优先使用在线编程环境，避免由于自己开发环境问题导致的bug**），除此之外，我们准备了详细的[本地开发环境准备文档](../dev-env/introduction.md)。

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

## 注意事项

1. **最重要的一条**：请不要将代码提交到公开仓库（包括提交带有题解的 Pull Request），同时也请不要抄袭其他同学或网络上可能存在的代码。
2. 实验题目都需要单人独立完成，不允许组队完成。
3. 这门课程需要你有一定的 C++ 编程基础。
4. 这门课程并不是关于如何使用数据库（例如如何写 SQL 语句，如何基于数据库构建上层应用）的课程。
5. 实验文档中提供了较为详细的指导，请优先仔细阅读文档。文档无法做到囊括所有问题的程度，如果还有问题也请随时提问。对于文档中的**注意** 提示，请认真阅读。对于文档中的 **思考** 提示，请结合实验内容思考，**思考** 不是实验课程作业的一部分，仅供个人学习，同时也不提供答案。

## C++ 编程语言学习

本门课程需要大家具备一定的 C++ 编程基础，这里推荐几个 C++ 基础学习的链接：

- [LearnCpp](https://www.learncpp.com/)(可以作为教程)
- Cpplings: `src/cpplings/README.md`(可以作为小练习)
- [cppreference](en.cppreference.com)(可以作为参考手册)