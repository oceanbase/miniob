---
title: Git 介绍
---

# Git 介绍
> Git 是一个开源的分布式版本控制系统，用于敏捷高效地处理任何或小或大的项目。
> 
> Git 是 Linus Torvalds 为了帮助管理 Linux 内核开发而开发的一个开放源码的版本控制软件。
> 

## 日常 Git 开发命令

- 查看当前分支

  ```bash
  git branch  # 查看本地分支

  git branch -a # 查看所有分支，包括远程分支
  ```

- 创建分支

  ```bash
  git checkout -b 'your branch name'

  git branch -d 'your branch name'  # 删除一个分支
  ```

- 切换分支

  ```bash
  git checkout 'branch name'
  ```

- 提交代码

  ```bash
  # 添加想要提交的文件或文件夹
  git add 'the files or directories you want to commit'
  # 这一步也可以用 git add . 添加当前目录

  # 提交到本地仓库
  # -m 中是提交代码的消息，建议写有意义的信息，方便后面查找
  git commit -m 'commit message'
  ```

- 推送代码到远程仓库

  ```bash
  git push
  # 可以将多次提交，一次性 push 到远程仓库
  ```

- 合并代码

  ```bash
  # 假设当前处于分支 develop 下
  git merge feature/update
  # 会将 feature/update 分支的修改，merge 到 develop 分支
  ```

- 临时修改另一个分支的代码

  ```bash
  # 有时候，正在开发一个新功能时，突然来了一个紧急 BUG，这时候需要切换到另一个分  去开发
  # 这时可以先把当前的代码提交上去，然后切换分支。
  # 或者也可以这样：
  git stash # 将当前的修改保存起来

  git checkout main # 切换到主分支，或者修复 BUG 的分支
 
  git checkout -b fix/xxx  # 创建一个新分支，用于修复问题

  # 修改完成后，merge 到 main 分支
  # 然后，继续我们的功能开发

  git checkout feature/update # 假设我们最开始就是在这个分支上
  git stash pop

  # stash 还有很多好玩的功能，大家可以探索一下
  ```
