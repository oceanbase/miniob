# Introduction
miniob 是 OceanBase与华中科技大学联合开发的、面向"零"基础数据库内核知识同学的一门数据库实现入门教程实践工具。
miniob设计的目标是让不熟悉数据库设计和实现的同学能够快速的了解与深入学习数据库内核，期望通过相关训练之后，能够对各个数据库内核模块的功能与它们之间的关联有所了解，并能够在
使用数据库时，设计出高效的SQL。面向的对象主要是在校学生，并且诸多模块做了简化，比如不考虑并发操作。
注意：此代码仅供学习使用，不考虑任何安全特性。

[GitHub 首页](https://github.com/oceanbase/miniob)

# How to build
直接在本地搭建开发环境，可以参考 [how_to_build](docs/how_to_build.md)。

# Docker

首先要确保本地已经安装了Docker。

- 使用 Dockerfile 构建

Dockerfile: https://github.com/oceanbase/miniob/blob/main/Dockerfile

```bash
# build
docker build -t miniob .
docker run -d --name='miniob' miniob:lastest

# 进入容器，开发调试
docker exec -it miniob bash
```

- 使用docker hub 镜像运行

```bash
docker run oceanbase/miniob
```

Docker环境说明：
docker基于`CentOS:7`制作。

镜像包含：

- jsoncpp
- google test
- libevent
- flex
- bison(3.7)
- gcc/g++ (version=11)
- miniob 源码(/root/source/miniob)

docker中在/root/source/miniob目录下载了github的源码，可以根据个人需要，下载自己仓库的源代码，也可以直接使用git pull 拉取最新代码。
/root/source/miniob/build.sh 提供了一个编译脚本，以DEBUG模式编译miniob。

# 数据库管理系统实现基础讲义
由华中科技大学谢美意和左琼老师联合编撰数据库管理系统实现教材。参考 [数据库管理系统实现基础讲义](docs/lectures/index.md)

# miniob 介绍
[miniob代码架构框架设计和说明](https://github.com/OceanBase-Partner/lectures-on-dbms-implementation/blob/main/miniob-introduction.md)

# miniob 题目
[miniob 题目](docs/miniob_topics.md)

# miniob 实现解析

[miniob-date 实现解析](https://oceanbase-partner.github.io/lectures-on-dbms-implementation/miniob-date-implementation.html)

[miniob drop-table 实现解析](https://oceanbase-partner.github.io/lectures-on-dbms-implementation/miniob-drop-table-implementation.html)

[miniob select-tables 实现解析](https://oceanbase-partner.github.io/lectures-on-dbms-implementation/miniob-select-tables-implementation.html)

[miniob 调试篇](https://oceanbase-partner.github.io/lectures-on-dbms-implementation/miniob-how-to-debug.html)
