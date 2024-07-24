---
title: 使用Docker开发MiniOB
---

本篇文章介绍如何使用Docker来开发MiniOB。

MiniOB 依赖的第三方组件比较多，搭建开发环境比较繁琐，建议同学们直接使用我们提供的Docker环境进行开发。

首先要确保本地已经安装了Docker。
如果对Docker还不太熟悉，可以先在网上大致了解一下。

我们提供了原始的Dockerfile，也有已经打包好的镜像，可以选择自己喜欢的方式。
自行构建参考[本文档](./how_to_dev_in_docker_container_by_vscode.md)。

- 拉取镜像

```bash
# 下面的命令三选一即可
docker pull oceanbase/miniob         # pull from docker hub
docker pull ghcr.io/oceanbase/miniob && docker tag ghcr.io/oceanbase/miniob oceanbase/miniob # pull from github
docker pull quay.io/oceanbase/miniob && docker tag quay.io/oceanbase/miniob oceanbase/miniob # pull from github # pull from quay.io
```

- 使用docker hub 镜像运行

```bash
docker run --privileged -d --name=miniob oceanbase/miniob
```
增加 privileged 权限是为了方便在容器中调试。

此命令会创建一个新的容器，然后可以执行下面的命令进入容器：

```bash
docker exec -it miniob /usr/bin/zsh
```

Docker环境说明：
docker基于`ubuntu:22.04`制作。

镜像包含：

- jsoncpp
- google test
- libevent
- flex
- bison(3.7)
- gcc/g++ (version=11)

