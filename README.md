# Introduction
miniob 是 OceanBase与华中科技大学联合开发的、面向"零"基础数据库内核知识同学的一门数据库实现入门教程实践工具。
miniob设计的目标是让不熟悉数据库设计和实现的同学能够快速的了解与深入学习数据库内核，期望通过相关训练之后，能够对各个数据库内核模块的功能与它们之间的关联有所了解，并能够在
使用数据库时，设计出高效的SQL。面向的对象主要是在校学生，并且诸多模块做了简化，比如不考虑并发操作。
注意：此代码仅供学习使用，不考虑任何安全特性。

[GitHub 首页](https://github.com/oceanbase/miniob)

# 如何开发
## 搭建开发环境
有多种方式搭建开发环境，可以直接在本地安装一些三方依赖，或者使用Docker。如果使用的是Windows，我们建议使用Docker来开发。

### 搭建本地开发环境
直接在本地搭建开发环境，可以参考 [how_to_build](docs/how_to_build.md)。

### 使用Docker开发

请参考 [如何使用Docker开发MiniOB](docs/how-to-dev-using-docker.md)

### Windows上开发MiniOB

[如何在Windows上使用Docker开发miniob](docs/how_to_dev_miniob_by_docker_on_windows.md)

## 词法语法解析开发环境

如果已经在处理一些SQL词法语法解析相关的问题，请参考 [MiniOB 词法语法解析开发与测试](docs/miniob-sql-parser.md)。
Docker 环境已经预安装了相关的组件。

# 数据库管理系统实现基础讲义
由华中科技大学谢美意和左琼老师联合编撰数据库管理系统实现教材。参考 [数据库管理系统实现基础讲义](docs/lectures/index.md)

# miniob 介绍
[miniob代码架构框架设计和说明](docs/miniob-introduction.md)

# miniob 训练
我们为MiniOB设计了配套的训练题目，大家可以在 [MiniOB 训练营](https://open.oceanbase.com/train?questionId=200001) 上进行提交测试。

[miniob 题目描述](docs/miniob_topics.md)

为了满足训练营或比赛测试要求，代码的输出需要满足一定要求，请参考 [MiniOB 输出约定](docs/miniob-output-convention.md)。一般情况下，不需要专门来看这篇文档，但是如果你的测试总是不正确，建议对照一下输出约定。

# miniob 实现解析

[miniob-date 实现解析](https://oceanbase-partner.github.io/lectures-on-dbms-implementation/miniob-date-implementation.html)

[miniob drop-table 实现解析](https://oceanbase-partner.github.io/lectures-on-dbms-implementation/miniob-drop-table-implementation.html)

[miniob select-tables 实现解析](https://oceanbase-partner.github.io/lectures-on-dbms-implementation/miniob-select-tables-implementation.html)

[miniob 调试篇](https://oceanbase-partner.github.io/lectures-on-dbms-implementation/miniob-how-to-debug.html)
