# miniob 概述

miniob 是 OceanBase 与华中科技大学联合开发的、面向"零"基础同学的数据库入门学习项目。

miniob 设计的目标是面向在校学生、数据库从业者、爱好者，或者对基础技术有兴趣的爱好者, 整体代码量少，易于上手并学习, 是一个系统性的数据库学习项目。miniob 设置了一系列由浅入深的题目，以帮助同学们"零"基础入门, 让同学们快速了解数据库并深入学习数据库内核，期望通过相关训练之后，能够熟练掌握数据库内核模块的功能与协同关系, 并能够在使用数据库时，设计出高效的 SQL 。miniob 为了更好的学习数据库实现原理, 对诸多模块都做了简化，比如不考虑并发操作, 安全特性, 复杂的事物管理等功能。

(注意：此代码仅供学习使用，请勿用于生产项目。)

## 快速上手

为了帮助开发者更好的上手并学习 miniob, 建议阅读：

1. [miniob 框架介绍](https://github.com/oceanbase/miniob/blob/main/docs/miniob-introduction.md)
2. [如何编译 miniob 源码](https://github.com/oceanbase/miniob/blob/main/docs/how_to_build.md)
3. [开发环境搭建(本地调试, 适用 Linux 和 Mac)](https://github.com/oceanbase/miniob/blob/main/docs/how_to_dev_miniob_by_vscode.md)
4. [开发环境搭建(远程调试, 适用于 Window, Linux 和 Mac)](https://github.com/oceanbase/miniob/blob/main/docs/how_to_dev_in_docker_container_by_vscode.md)
5. [miniob 词法语法解析开发与测试](https://github.com/oceanbase/miniob/blob/main/docs/miniob-sql-parser.md)

更多的文档, 可以参考 [docs](https://github.com/oceanbase/miniob/tree/main/docs), 为了帮助大家更好的学习数据库基础知识, OceanBase 社区提供了一系列教程, 建议学习:

1. [《从0到1数据库内核实战教程》  视频教程](https://open.oceanbase.com/activities/4921877?id=4921946)
2. [《从0到1数据库内核实战教程》  基础讲义](https://github.com/oceanbase/kernel-quickstart)
3. [《数据库管理系统实现》  华中科技大学实现教材](https://github.com/oceanbase/miniob/blob/main/docs/lectures/index.md)

## 系统架构

miniob 整体架构如下图所示:
![架构](https://github.com/oceanbase/miniob/blob/main/docs/images/miniob-introduction-sql-flow.png?raw=true)

其中:

- 网络模块：负责与客户端交互，收发客户端请求与应答；

- SQL解析：将用户输入的SQL语句解析成语法树；

- 执行计划缓存：执行计划缓存模块会将该 SQL第一次生成的执行计划缓存在内存中，后续的执行可以反复执行这个计划，避免了重复查询优化的过程（未实现）。

- 语义解析模块：将生成的语法树，转换成数据库内部数据结构（部分实现）；

- 查询缓存：将执行的查询结果缓存在内存中，下次查询时，可以直接返回（未实现）；

- 查询优化：根据一定规则和统计数据，调整/重写语法树。当前实现为空，留作实验题目；

- 计划执行：根据语法树描述，执行并生成结果；

- 会话管理：管理用户连接、调整某个连接的参数；

- 元数据管理：记录当前的数据库、表、字段和索引元数据信息；

- 客户端：作为测试工具，接收用户请求，向服务端发起请求。


## OceanBase 大赛

2022 OceanBase 数据库大赛是由中国计算机学会（CCF）数据库专业委员会指导，OceanBase 与蚂蚁技术研究院学术合作团队联合举办的数据库内核实战赛事。本次大赛主要面向全国爱好数据库的高校学生，以“竞技、交流、成长”为宗旨，搭建基于赛事的技术交流平台，促进高校创新人才培养机制，不仅帮助学生从0开始系统化学习数据库理论知识，提升学生数据库实践能力，更能帮助学生走向企业积累经验，促进国内数据库人才的发展，碰撞出创新的火花。

OceanBase 初赛基于一套适合初学者实践的数据库实训平台 miniob，代码量少，易于上手学习，包含了数据库的各个关键模块，是一个系统性的数据库学习平台。基于该平台设置了一系列由浅入深的题目，以帮助同学们更好"零"基础入门。

更多详情, 请参考 [OceanBase 大赛](https://open.oceanbase.com/competition/index)

### 1. 大赛手把手入门教程

[大赛入门教程](https://github.com/oceanbase/kernel-quickstart/blob/V1.0.0/zh-CN/1.database-system-overview/5.miniob-github-gitee-instructions.md)

### 2. 大赛赛题

[赛题介绍](https://github.com/oceanbase/miniob/blob/main/docs/miniob_topics.md) 

### 3. 提交测试

题目完成并通过自测后，大家可以在 [miniob 训练营](https://open.oceanbase.com/train?questionId=500003) 上提交代码进行测试。

在提交前, 请参考并学习 [训练营使用说明](https://ask.oceanbase.com/t/topic/35600372)

客户端输出需要满足一定要求，如果你的测试结果不符合预期，请参考 [miniob 输出约定](https://github.com/oceanbase/miniob/blob/main/docs/miniob-output-convention.md)。

### 4. 大赛FAQ

[大赛 FAQ ](https://ask.oceanbase.com/t/topic/35601465)

## Contributing

OceanBase 社区热情欢迎每一位对数据库技术热爱的开发者，期待携手开启思维碰撞之旅。无论是文档格式调整或文字修正、问题修复还是增加新功能，都是对 OceanBase 社区参与和贡献方式之一，立刻开启您的 First Contribution 吧！更多详情, 请参考 [社区贡献](https://github.com/oceanbase/miniob/blob/main/CONTRIBUTING.md).

## License

miniob 采用 [木兰宽松许可证，第2版](https://license.coscl.org.cn/MulanPSL2), 可以自由拷贝和使用源码, 当做修改或分发时, 请遵守 [木兰宽松许可证，第2版](https://license.coscl.org.cn/MulanPSL2). 

## 社区组织

- [OceanBase 社区交流群 33254054](https://h5.dingtalk.com/circle/healthCheckin.html?corpId=dingd88359ef5e4c49ef87cda005313eea7a&1fe0ca69-72d=16c86a07-83c&cbdbhh=qwertyuiop&origin=1)
- [OceanBase 大赛官方交流群 35326455](https://qr.dingtalk.com/action/joingroup?code=v1,k1,g61jI0RwHQA8UMocuTbys2cyM7vck2c6jNE87vdxz9o=&_dt_no_comment=1&origin=11)