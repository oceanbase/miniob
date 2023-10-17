# MiniOB 概述

MiniOB 是 [OceanBase](https://github.com/oceanbase/oceanbase) 团队基于华中科技大学数据库课程原型，联合多所高校重新开发的、专为零基础的同学设计的数据库入门学习项目。我们的目标是为在校学生、数据库从业者、爱好者或对基础技术感兴趣的人提供一个友好的数据库学习项目。

MiniOB 整体代码简洁，容易上手，设计了一系列由浅入深的题目，帮助同学们从零基础入门，迅速了解数据库并深入学习数据库内核。MiniOB 简化了许多模块，例如不考虑并发操作、安全特性和复杂的事务管理等功能，以便更好地学习数据库实现原理。我们期望通过 MiniOB 的训练，同学们能够熟练掌握数据库内核模块的功能和协同关系，并具备一定的工程编码能力，例如内存管理、网络通信和磁盘 I/O 处理等, 这将有助于同学在未来的面试和工作中脱颖而出。

# [文档](https://oceanbase.github.io/miniob/)
代码配套文档和相关代码注释已经生成文档，并通过 GitHub Pages 发布。您可以直接访问：[MiniOB GitHub Pages](https://oceanbase.github.io/miniob/).

## 快速上手

为了帮助开发者更好地上手并学习 MiniOB，建议阅读以下内容：

1. [MiniOB 框架介绍](https://oceanbase.github.io/miniob/miniob-introduction.html)
2. [如何编译 MiniOB 源码](https://oceanbase.github.io/miniob/how_to_build.html)
3. [如何运行 MiniOB](https://oceanbase.github.io/miniob/how_to_run.html)
4. [使用 GitPod 开发 MiniOB](https://oceanbase.github.io/miniob/dev-env/dev_by_gitpod.html)
5. [doxygen 代码文档](https://oceanbase.github.io/miniob/design/doxy/html/index.html)

为了帮助大家更好地学习数据库基础知识，OceanBase社区提供了一系列教程。更多文档请参考 [MiniOB GitHub Pages](https://oceanbase.github.io/miniob/)。建议学习：

1. [《从0到1数据库内核实战教程》  视频教程](https://open.oceanbase.com/activities/4921877?id=4921946)
2. [《从0到1数据库内核实战教程》  基础讲义](https://github.com/oceanbase/kernel-quickstart)
3. [《数据库管理系统实现》  华中科技大学实现教材](https://oceanbase.github.io/miniob/lectures/index.html)

## 系统架构

MiniOB 整体架构如下图所示:
![架构](docs/src/images/miniob-introduction-sql-flow.png)

其中:

- 网络模块：负责与客户端交互，收发客户端请求与应答；

- SQL解析：将用户输入的SQL语句解析成语法树；

- 执行计划缓存：执行计划缓存模块会将该 SQL第一次生成的执行计划缓存在内存中，后续的执行可以反复执行这个计划，避免了重复查询优化的过程（未实现）。

- 语义解析模块：将生成的语法树，转换成数据库内部数据结构（部分实现）；

- 查询缓存：将执行的查询结果缓存在内存中，下次查询时，可以直接返回（未实现）；

- 查询优化：根据一定规则和统计数据，调整/重写语法树。(部分实现)；

- 计划执行：根据语法树描述，执行并生成结果；

- 会话管理：管理用户连接、调整某个连接的参数；

- 元数据管理：记录当前的数据库、表、字段和索引元数据信息；

- 客户端：作为测试工具，接收用户请求，向服务端发起请求。


# [OceanBase 大赛](https://open.oceanbase.com/competition)

2022 OceanBase 数据库大赛是由中国计算机学会（CCF）数据库专业委员会指导，OceanBase 与蚂蚁技术研究院学术合作团队联合举办的数据库内核实战赛事。本次大赛主要面向全国爱好数据库的高校学生，以“竞技、交流、成长”为宗旨，搭建基于赛事的技术交流平台，促进高校创新人才培养机制，不仅帮助学生从0开始系统化学习数据库理论知识，提升学生数据库实践能力，更能帮助学生走向企业积累经验，促进国内数据库人才的发展，碰撞出创新的火花。

OceanBase 初赛基于一套适合初学者实践的数据库实训平台 MiniOB，代码量少，易于上手学习，包含了数据库的各个关键模块，是一个系统性的数据库学习平台。基于该平台设置了一系列由浅入深的题目，以帮助同学们更好"零"基础入门。

2023 OceanBase 数据库大赛正在火热进行, 更多详情, 请参考 [OceanBase 大赛](https://open.oceanbase.com/competition/index)。

### 1. 大赛手把手入门教程

[大赛入门教程](https://oceanbase.github.io/miniob/game/gitee-instructions.html)

### 2. 大赛赛题

[赛题介绍](https://oceanbase.github.io/miniob/game/miniob_topics.html) 

### 3. 提交测试

题目完成并通过自测后，大家可以在 [MiniOB 训练营](https://open.oceanbase.com/train?questionId=500003) 上提交代码进行测试。

在提交前, 请参考并学习 [训练营使用说明](https://ask.oceanbase.com/t/topic/35600372)。

### 4. 大赛FAQ

[大赛 FAQ ](https://ask.oceanbase.com/t/topic/35601465)

# 在线开发平台

本仓库基于 Gitpod 建立了快速在线开发平台。点击下面的按钮即可一键体验（建议使用 Chrome 浏览器）。

[![Open in Gitpod](https://gitpod.io/button/open-in-gitpod.svg)](https://gitpod.io/#https://github.com/oceanbase/miniob)

首次进入 Gitpod 时需要安装一些依赖。安装完成后，终端界面会显示 "Dependency installed successfully"。运行 `bash build.sh --make -j4` 命令即可编译 MiniOB。

# Contributing

OceanBase 社区热情欢迎每一位对数据库技术热爱的开发者，期待与您携手开启思维碰撞之旅。无论是文档格式调整或文字修正、问题修复还是增加新功能，都是参与和贡献 OceanBase 社区的方式之一。现在就开始您的首次贡献吧！更多详情，请参考 [社区贡献](CONTRIBUTING.md).

# License

MiniOB 采用 [木兰宽松许可证，第2版](https://license.coscl.org.cn/MulanPSL2), 可以自由拷贝和使用源码, 当做修改或分发时, 请遵守 [木兰宽松许可证，第2版](https://license.coscl.org.cn/MulanPSL2). 

# 社区组织

- [OceanBase 社区交流群 33254054](https://h5.dingtalk.com/circle/healthCheckin.html?corpId=dingd88359ef5e4c49ef87cda005313eea7a&1fe0ca69-72d=16c86a07-83c&cbdbhh=qwertyuiop&origin=1)
- [OceanBase 大赛官方交流群 35326455](https://qr.dingtalk.com/action/joingroup?code=v1,k1,g61jI0RwHQA8UMocuTbys2cyM7vck2c6jNE87vdxz9o=&_dt_no_comment=1&origin=11)
- [OceanBase 官方论坛](https://ask.oceanbase.com/)
