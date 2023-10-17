# OceanBase 贡献指南

OceanBase 社区热情欢迎每一位对数据库技术热爱的开发者，期待携手开启思维碰撞之旅。无论是文档格式调整或文字修正、问题修复还是增加新功能，都是对 OceanBase 社区参与和贡献方式之一，立刻开启您的 First Contribution 吧！

## 阅读建议

为了帮助开发者更好的上手并学习 miniob, 建议阅读：

1. [miniob 框架介绍](docs/src/miniob-introduction.md)
2. [如何编译 miniob 源码](docs/src/how_to_build.md)
3. [开发环境搭建(本地调试, 适用 Linux 和 Mac)](docs/src/dev-env/how_to_dev_miniob_by_vscode.md)
4. [开发环境搭建(远程调试, 适用于 Window, Linux 和 Mac)](docs/src/dev-env/how_to_dev_in_docker_container_by_vscode.md)
5. [miniob 文档汇总](docs/src/SUMMARY.md)

更多的文档, 可以参考 [docs](https://github.com/oceanbase/miniob/tree/main/docs), 为了帮助大家更好的学习数据库基础知识, OceanBase 社区提供了一系列教程, 建议学习:

1. [《从0到1数据库内核实战教程》  视频教程](https://open.oceanbase.com/activities/4921877?id=4921946)
2. [《从0到1数据库内核实战教程》  基础讲义](https://github.com/oceanbase/kernel-quickstart)
3. [《数据库管理系统实现》  华中科技大学实现教材](docs/src/lectures/index.md)

## 如何找到一个合适issue

* 通过[寻找一个合适的issue](https://github.com/oceanbase/miniob/issues), 找到一个可以入手的issue
* 通过`bug`/`new feature`找到当前版本的bug和建议添加的功能
找到合适的issue之后，可以在issue下回复`/assign` 将issue分配给自己，并添加`developing`标签，这个标签表示当前这个issue在编码开发中

## 代码贡献流程

以 Mac 操作系统为例

### 1. Fork 项目仓库

1.  访问项目的 [GitHub 地址](https://github.com/oceanbase/miniob)。 
2.  点击 Fork 按钮创建远程分支。

### 2. 配置本地环境变量

```
working_dir=$HOME/workspace # 定义工作目录
user={GitHub账户名} # 和github上的用户名保持一致
```

### 3. 克隆代码

```
mkdir -p $working_dir
cd $working_dir
git clone git@github.com:$user/miniob.git

# 添加上游分支
cd $working_dir/miniob
git remote add upstream git@github.com:oceanbase/miniob.git

# 为上游分支设置 no_push
git remote set-url --push upstream no_push

# 确认远程分支有效
git remote -v
```

### 4. 创建新分支

```
# 更新本地 master 分支。 
new_branch_name={issue_xxx} # 设定分支名，建议直接使用issue+id的命名
cd $working_dir/oceanbase
git fetch upstream
git checkout main
git rebase upstream/main
git checkout -b $new_branch_name
```

### 5. 开发

在新建的分支上完成开发

### 6. 提交代码

```
# 检查本地文件状态
git status

# 添加您希望提交的文件
# 如果您希望提交所有更改，直接使用 `git add .`
git add <file> ... 
# 为了让 github 自动将 pull request 关联上 github issue, 
# 建议 commit message 中带上 " #{issueid}", 其中{issueid} 为issue 的id, 
git commit -m "commit-message: update the xx"

# 在开发分支执行以下操作
git fetch upstream
git rebase upstream/main
git push -u origin $new_branch_name
```

### 7. 创建 PR

1.  访问您 Fork 的仓库。 
2.  单击 {new_branch_name} 分支旁的 Compare & pull request 按钮。

### 8. 签署 CLA 协议
签署[Contributor License Agreement (CLA)](https://cla-assistant.io/oceanbase/oceanbase) ；在提交 Pull Request 的过程中需要签署后才能进入下一步流程。如果没有签署，在提交流程会有如下报错：

![image](https://user-images.githubusercontent.com/5435903/204097095-6a19d2d1-ee0c-4fb6-be2d-77f7577d75d2.png)

### 9. 代码审查与合并
有review、合并权限的维护者，会帮助开发者进行代码review；review意见通过后，后续的操作都会由维护者进行，包括运行CI测试（目前包括centos和ubuntu的编译）等，最终代码会有维护者通过后合入.

### 10. 祝贺成为贡献者

当 pull request 合并后, 则所有的 contributing 工作全部完成, 恭喜您, 您成为 OceanBase 贡献者.
