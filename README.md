# MiniOB 介绍

<div align="left">

[![Chinese Doc](https://img.shields.io/badge/文档-简体中文-blue)](https://oceanbase.github.io/miniob/)
[![MiniOB stars](https://img.shields.io/badge/dynamic/json?color=blue&label=stars&query=stargazers_count&url=https%3A%2F%2Fapi.github.com%2Frepos%2Foceanbase%2Fminiob)](https://github.com/oceanbase/miniob)
[![Coverage Status](https://codecov.io/gh/oceanbase/miniob/branch/main/graph/badge.svg)](https://codecov.io/gh/oceanbase/miniob)
[![HelloGitHub](https://abroad.hellogithub.com/v1/widgets/recommend.svg?rid=62efc8a5bbb64a9fbb1ebb7703446f36&claim_uid=AptH8D2YM3rCGL9&theme=small)](https://hellogithub.com/repository/62efc8a5bbb64a9fbb1ebb7703446f36)

</div>

MiniOB 是 [OceanBase](https://github.com/oceanbase/oceanbase) 团队基于华中科技大学数据库课程原型，联合多所高校重新开发的、专为零基础的同学设计的数据库入门学习项目。MiniOB 的目标是为在校学生、数据库从业者、爱好者或对基础技术感兴趣的人提供一个友好的数据库学习项目，更好地将理论、实践进行结合，提升同学们的工程实战能力。

MiniOB 整体代码简洁，容易上手，设计了一系列由浅入深的题目，帮助同学们从零基础入门，迅速了解数据库并深入学习数据库内核。MiniOB 简化了许多模块，例如不考虑并发操作、安全特性和复杂的事务管理等功能，以便更好地学习数据库实现原理。我们期望通过 MiniOB 的训练，同学们能够熟练掌握数据库内核模块的功能和协同关系，并具备一定的工程编码能力，例如内存管理、网络通信和磁盘 I/O 处理等, 这将有助于同学在未来的面试和工作中脱颖而出。

# [文档](https://oceanbase.github.io/miniob/)
代码配套设计文档和相关代码注释已经生成文档，并通过 GitHub Pages 发布。您可以直接访问：[MiniOB GitHub Pages](https://oceanbase.github.io/miniob/).

## 快速上手

为了帮助开发者更好地上手并学习 MiniOB，建议阅读以下内容：

1. [MiniOB 框架介绍](https://oceanbase.github.io/miniob/design/miniob-architecture/)
2. [如何编译 MiniOB 源码](https://oceanbase.github.io/miniob/how_to_build/)
3. [如何运行 MiniOB](https://oceanbase.github.io/miniob/how_to_run/)
4. [使用 GitPod 开发 MiniOB](https://oceanbase.github.io/miniob/dev-env/dev_by_gitpod/)
5. [doxygen 代码文档](https://oceanbase.github.io/miniob/design/doxy/html/index.html)

为了帮助大家更好地学习数据库基础知识，OceanBase社区提供了一系列教程。更多文档请参考 [MiniOB GitHub Pages](https://oceanbase.github.io/miniob/)。建议学习：

1. [《从0到1数据库内核实战教程》  视频教程](https://open.oceanbase.com/activities/4921877?id=4921946)
2. [《从0到1数据库内核实战教程》  基础讲义](https://github.com/oceanbase/kernel-quickstart)
3. [《数据库管理系统实现》  华中科技大学实现教材](https://oceanbase.github.io/miniob/lectures/index.html)

## 系统架构

MiniOB 整体架构如下图所示:

<img src="./docs/docs/design/images/miniob-architecture.svg" width = "60%" alt="InternalNode" align=center />

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

# 在线开发平台

搭建开发环境是一个比较耗时而且繁琐的事情，特别是对于初学者。为了让大家更快地上手 MiniOB，本仓库基于 Gitpod 建立了快速在线开发平台。点击下面的按钮即可一键体验（建议使用 Chrome 浏览器）。

[![Open in Gitpod](https://gitpod.io/button/open-in-gitpod.svg)](https://gitpod.io/#https://github.com/oceanbase/miniob)

> 注意：这个链接打开的是MiniOB主仓库的开发环境，同学们需要通过gitpod进入自己的仓库开发环境。

首次进入 Gitpod 时需要安装一些依赖。安装完成后，终端界面会显示 "Dependency installed successfully"。运行 `bash build.sh --make -j4` 命令即可编译 MiniOB。

# Contributing

OceanBase 社区热情欢迎每一位对数据库技术热爱的开发者，期待与您携手开启思维碰撞之旅。无论是文档格式调整或文字修正、问题修复还是增加新功能，都是参与和贡献 OceanBase 社区的方式之一。MiniOB 现在开放了一些[新功能的开发](https://github.com/oceanbase/miniob/issues?q=is%3Aopen+is%3Aissue+label%3A%22help+wanted%22)，欢迎有兴趣的同学一起共建，希望我们共同成长。如果你对MiniOB不熟悉也没关系，可以直接联系我们，我们将会有人指导上手。现在就开始您的首次贡献吧！更多详情，请参考 [社区贡献](CONTRIBUTING.md)。

# Contributors
感谢所有为 MiniOB 项目做出贡献的同学们！

<a href="https://github.com/oceanbase/miniob/graphs/contributors"><img src="https://contributors-img.web.app/image?repo=oceanbase/miniob&width=890" /></a>

# License

MiniOB 采用 [木兰宽松许可证，第2版](https://license.coscl.org.cn/MulanPSL2), 可以自由拷贝和使用源码, 当做修改或分发时, 请遵守 [木兰宽松许可证，第2版](https://license.coscl.org.cn/MulanPSL2). 

# 社区组织

- [OceanBase 社区交流群 33254054](https://h5.dingtalk.com/circle/healthCheckin.html?corpId=dingd88359ef5e4c49ef87cda005313eea7a&1fe0ca69-72d=16c86a07-83c&cbdbhh=qwertyuiop&origin=1)
- [OceanBase 大赛官方交流群 35326455](https://qr.dingtalk.com/action/joingroup?code=v1,k1,g61jI0RwHQA8UMocuTbys2cyM7vck2c6jNE87vdxz9o=&_dt_no_comment=1&origin=11)
- [OceanBase 官方论坛](https://ask.oceanbase.com/)
- MiniOB 开发者微信群(添加 hnwyllmm_126 为好友，备注 MiniOB，邀请入群)
